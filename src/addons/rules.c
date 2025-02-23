#include "../private_api.h"

#ifdef FLECS_RULES

#include <stdio.h>

/** Implementation of the rule query engine.
 * 
 * A rule (terminology borrowed from prolog) is a list of constraints that 
 * specify which conditions must be met for an entity to match the rule. While
 * this description matches any kind of ECS query, the rule engine has features
 * that go beyond regular (flecs) ECS queries:
 * 
 * - query for all components of an entity (vs. all entities for a component)
 * - query for all relationship pairs of an entity
 * - support for query variables that are resolved at evaluation time
 * - automatic traversal of transitive relationships
 * 
 * Query terms can have the following forms:
 * 
 * - Component(Subject)
 * - Relation(Subject, Object)
 * 
 * Additionally the query parser supports the following shorthand notations:
 * 
 * - Component             // short for Component(This)
 * - (Relation, Object)    // short for Relation(This, Object)
 * 
 * The subject, or first arugment of a term represents the entity on which the
 * component or relation is matched. By default the subject is set to a builtin
 * This variable, which causes the behavior to match a regular ECS query:
 * 
 * - Position, Velocity
 * 
 * Is equivalent to
 *
 * - Position(This), Velocity(This)
 * 
 * The function of the variable is to ensure that all components are matched on
 * the same entity. Conceptually the query first populates the This variable
 * with all entities that have Position. When the query evaluates the Velocity
 * term, the variable is populated and the entity it contains will be checked
 * for whether it has Velocity.
 * 
 * The actual implementation is more efficient and does not check per-entity.
 * 
 * Custom variables can be used to join parts of different terms. For example, 
 * the following query can be used to find entities with a parent that has a
 * Position component (note that variable names start with a _):
 * 
 * - ChildOf(This, _Parent), Component(_Parent)
 * 
 * The rule engine uses a backtracking algorithm to find the set of entities
 * and variables that match all terms. As soon as the engine finds a term that
 * does not match with the currently evaluated entity, the entity is discarded.
 * When an entity is found for which all terms match, the entity is yielded to
 * the iterator.
 * 
 * While a rule is being evaluated, a variable can either contain a single 
 * entity or a table. The engine will attempt to work with tables as much as
 * possible so entities can be eliminated/yielded in bulk. A rule may store
 * both the table and entity version of a variable and only switch from table to
 * entity when necessary.
 * 
 * The rule engine has an algorithm for computing which variables should be 
 * resolved first. This algorithm works by finding a "root" variable, which is
 * the subject variable that occurs in the term with the least dependencies. The
 * remaining variables are then resolved based on their "distance" from the root
 * with the closest variables being resolved first.
 * 
 * This generally results in an ordering that resolves the variables with the 
 * least dependencies first and the most dependencies last, which is beneficial
 * for two reasons:
 * 
 * - it improves the average performance of all queries
 * - it makes performance less dependent on how an application orders the terms
 * 
 * A possible improvement would be for the query engine to also consider
 * the number of tables that need to be evaluated for each term, as starting
 * with the smallest term reduces the amount of work. Other than static variable
 * analysis however, this can only be determined when the query is executed.
 * 
 * Rules are "compiled" into a set of instructions that encode the operations
 * the query needs to perform in order to find the right set of entities.
 * Operations can either yield data, which progresses the program, or signal
 * that there is no (more) matching data, which discards the current variables.
 * 
 * An operation can yield multiple times, if there are multiple matches for its
 * inputs. Operations are called with a redo flag, which can be either true or
 * false. When redo is true the operation will yield the next result. When redo
 * is false, the operation will reset its state and start from the first result.
 * 
 * Operations can have an input, output and a filter. Most commonly an operation
 * either matches the filter against an input and yields if it matches, or uses
 * the filter to find all matching results and store the result in the output.
 *
 * Variables are resolved by matching a filter against the output of an 
 * operation. When a term contains variables, they are encoded as register ids
 * in the filter. When the filter is evaluated, the most recent values of the
 * register are used to match/lookup the output.
 * 
 * For example, a filter could be (ChildOf, _Parent). When the program starts,
 * the _Parent register is initialized with *, so that when this filter is first
 * evaluated, the operation will find all tables with (ChildOf, *). The _Parent
 * register is then populated by taking the actual value of the table. If the
 * table has type [(ChildOf, Sun)], _Parent will be initialized with Sun.
 * 
 * It is possible that a filter matches multiple times. Consider the filter
 * (Likes, _Food), and a table [(Likes, Apples), (Likes, Pears)]. In this case
 * an operation will yield the table twice, once with _Food=Apples, and once
 * with _Food=Pears.
 * 
 * If a rule contains a term with a transitive relation, it will automatically
 * substitute the parts of the term to find a fact that matches. The following
 * examples illustrate how transitivity is resolved:
 * 
 * Query:
 *   LocatedIn(Bob, SanFrancisco)
 *   
 * Expands to:
 *   LocatedIn(Bob, SanFrancisco:self|subset)
 * 
 * Explanation:
 *   "Is Bob located in San Francisco" - This term is true if Bob is either 
 *   located in San Francisco, or is located in anything that is itself located 
 *   in (a subset of) San Francisco.
 * 
 * 
 * Query:
 *   LocatedIn(Bob, X)
 * 
 * Expands to:
 *   LocatedIn(Bob, X:self|superset)
 * 
 * Explanation:
 *   "Where is Bob located?" - This term recursively returns all places that
 *   Bob is located in, which includes his location and the supersets of his
 *   location. When Bob is located in San Francisco, he is also located in
 *   the United States, North America etc.
 * 
 * 
 * Query:
 *   LocatedIn(X, NorthAmerica)
 * 
 * Expands to:
 *   LocatedIn(X, NorthAmerica:self|subset)
 * 
 * Explanation:
 *   "What is located in North America?" - This term returns everything located
 *   in North America and its subsets, as something located in San Francisco is
 *   located in UnitedStates, which is located in NorthAmerica. 
 * 
 * 
 * Query:
 *   LocatedIn(X, Y)
 * 
 * Expands to:
 *   LocatedIn(X, Y)
 * 
 * Explanation:
 *   "Where is everything located" - This term returns everything that is
 *   located somewhere. No substitution is performed as this would explode the
 *   results while not yielding new information.
 * 
 * 
 * In the above terms, the variable indicates the part of the term that is
 * unknown at evaluation time. In an actual rule the picked strategy depends on
 * whether the variable is known when the term is evaluated. For example, if
 * variable X has been resolved by the time Located(X, Y) is evaluated, the
 * strategy from the LocatedIn(Bob, X) example will be used.
 */

#define ECS_RULE_MAX_VARIABLE_COUNT (256)

#define RULE_PAIR_PREDICATE (1)
#define RULE_PAIR_OBJECT (2)

/* A rule pair contains a predicate and object that can be stored in a register. */
typedef struct ecs_rule_pair_t {
    union {
        int32_t reg;
        ecs_entity_t ent;
    } pred;
    union {
        int32_t reg;
        ecs_entity_t ent;
    } obj;
    int32_t reg_mask; /* bit 1 = predicate, bit 2 = object, bit 4 = wildcard */
    bool transitive; /* Is predicate transitive */
    bool final;      /* Is predicate final */
    bool inclusive;  /* Is predicate inclusive */
    bool obj_0;
} ecs_rule_pair_t;

/* Filter for evaluating & reifing types and variables. Filters are created ad-
 * hoc from pairs, and take into account all variables that had been resolved
 * up to that point. */
typedef struct ecs_rule_filter_t {
    ecs_id_t mask;  /* Mask with wildcard in place of variables */

    bool wildcard; /* Does the filter contain wildcards */
    bool pred_wildcard; /* Is predicate a wildcard */
    bool obj_wildcard; /* Is object a wildcard */
    bool same_var; /* True if pred & obj are both the same variable */

    int32_t hi_var; /* If hi part should be stored in var, this is the var id */
    int32_t lo_var; /* If lo part should be stored in var, this is the var id */
} ecs_rule_filter_t;

/* A rule register stores temporary values for rule variables */
typedef enum ecs_rule_var_kind_t {
    EcsRuleVarKindTable, /* Used for sorting, must be smallest */
    EcsRuleVarKindEntity,
    EcsRuleVarKindUnknown
} ecs_rule_var_kind_t;

typedef struct ecs_rule_reg_t {
    /* Used for table variable */
    ecs_table_t *table;
    int32_t offset;
    int32_t count;

    /* Used for entity variable. May also be set for table variable if it needs
     * to store an empty entity. */
    ecs_entity_t entity;
} ecs_rule_reg_t;
 
/* Operations describe how the rule should be evaluated */
typedef enum ecs_rule_op_kind_t {
    EcsRuleInput,       /* Input placeholder, first instruction in every rule */
    EcsRuleSelect,      /* Selects all ables for a given predicate */
    EcsRuleWith,        /* Applies a filter to a table or entity */
    EcsRuleSubSet,      /* Finds all subsets for transitive relationship */
    EcsRuleSuperSet,    /* Finds all supersets for a transitive relationship */
    EcsRuleStore,       /* Store entity in table or entity variable */
    EcsRuleEach,        /* Forwards each entity in a table */
    EcsRuleSetJmp,      /* Set label for jump operation to one of two values */
    EcsRuleJump,        /* Jump to an operation label */
    EcsRuleNot,         /* Invert result of an operation */
    EcsRuleYield        /* Yield result */
} ecs_rule_op_kind_t;

/* Single operation */
typedef struct ecs_rule_op_t {
    ecs_rule_op_kind_t kind;    /* What kind of operation is it */
    ecs_rule_pair_t filter;     /* Parameter that contains optional filter */
    ecs_entity_t subject;       /* If set, operation has a constant subject */

    int32_t on_pass;            /* Jump location when match succeeds */
    int32_t on_fail;            /* Jump location when match fails */
    int32_t frame;              /* Register frame */

    int32_t term;               /* Corresponding term index in signature */
    int32_t r_in;               /* Optional In/Out registers */
    int32_t r_out;

    bool has_in, has_out;       /* Keep track of whether operation uses input
                                 * and/or output registers. This helps with
                                 * debugging rule programs. */
} ecs_rule_op_t;

/* With context. Shared with select. */
typedef struct ecs_rule_with_ctx_t {
    ecs_id_record_t *idr;      /* Currently evaluated table set */
    int32_t table_index;
} ecs_rule_with_ctx_t;

/* Subset context */
typedef struct ecs_rule_subset_frame_t {
    ecs_rule_with_ctx_t with_ctx;
    ecs_table_t *table;
    int32_t row;
    int32_t column;
} ecs_rule_subset_frame_t;

typedef struct ecs_rule_subset_ctx_t {
    ecs_rule_subset_frame_t storage[16]; /* Alloc-free array for small trees */
    ecs_rule_subset_frame_t *stack;
    int32_t sp;
} ecs_rule_subset_ctx_t;

/* Superset context */
typedef struct ecs_rule_superset_frame_t {
    ecs_table_t *table;
    int32_t column;
} ecs_rule_superset_frame_t;

typedef struct ecs_rule_superset_ctx_t {
    ecs_rule_superset_frame_t storage[16]; /* Alloc-free array for small trees */
    ecs_rule_superset_frame_t *stack;
    ecs_id_record_t *idr;
    int32_t sp;
} ecs_rule_superset_ctx_t;

/* Each context */
typedef struct ecs_rule_each_ctx_t {
    int32_t row;                /* Currently evaluated row in evaluated table */
} ecs_rule_each_ctx_t;

/* Jump context */
typedef struct ecs_rule_setjmp_ctx_t {
    int32_t label;             /* Operation label to jump to */
} ecs_rule_setjmp_ctx_t;

/* Operation context. This is a per-operation, per-iterator structure that
 * stores information for stateful operations. */
typedef struct ecs_rule_op_ctx_t {
    union {
        ecs_rule_subset_ctx_t subset;
        ecs_rule_superset_ctx_t superset;
        ecs_rule_with_ctx_t with;
        ecs_rule_each_ctx_t each;
        ecs_rule_setjmp_ctx_t setjmp;
    } is;
} ecs_rule_op_ctx_t;

/* Rule variables allow for the rule to be parameterized */
typedef struct ecs_rule_var_t {
    ecs_rule_var_kind_t kind;
    char *name;       /* Variable name */
    int32_t id;       /* Unique variable id */
    int32_t occurs;   /* Number of occurrences (used for operation ordering) */
    int32_t depth;  /* Depth in dependency tree (used for operation ordering) */
    bool marked;      /* Used for cycle detection */
} ecs_rule_var_t;

/* Top-level rule datastructure */
struct ecs_rule_t {
    ecs_header_t hdr;
    
    ecs_world_t *world;         /* Ref to world so rule can be used by itself */
    ecs_rule_op_t *operations;  /* Operations array */
    ecs_rule_var_t *variables;  /* Variable array */
    ecs_filter_t filter;        /* Filter of rule */

    char **variable_names;      /* Array with var names, used by iterators */
    int32_t *subject_variables; /* Variable id for term subject (if any) */

    int32_t variable_count;     /* Number of variables in signature */
    int32_t subject_variable_count;
    int32_t frame_count;        /* Number of register frames */
    int32_t operation_count;    /* Number of operations in rule */

    ecs_iterable_t iterable;    /* Iterable mixin */
};

/* ecs_rule_t mixins */
ecs_mixins_t ecs_rule_t_mixins = {
    .type_name = "ecs_rule_t",
    .elems = {
        [EcsMixinWorld] = offsetof(ecs_rule_t, world),
        [EcsMixinIterable] = offsetof(ecs_rule_t, iterable)
    }
};

static
void rule_error(
    const ecs_rule_t *rule,
    const char *fmt,
    ...)
{
    va_list valist;
    va_start(valist, fmt);
    ecs_parser_errorv(rule->filter.name, rule->filter.expr, -1, fmt, valist);
    va_end(valist);
}

static
bool subj_is_set(
    ecs_term_t *term)
{
    return ecs_term_id_is_set(&term->subj);
}

static
bool obj_is_set(
    ecs_term_t *term)
{
    return ecs_term_id_is_set(&term->obj) || term->role == ECS_PAIR;
}

static
ecs_rule_op_t* create_operation(
    ecs_rule_t *rule)
{
    int32_t cur = rule->operation_count ++;
    rule->operations = ecs_os_realloc(
        rule->operations, (cur + 1) * ECS_SIZEOF(ecs_rule_op_t));

    ecs_rule_op_t *result = &rule->operations[cur];
    ecs_os_memset_t(result, 0, ecs_rule_op_t);

    return result;
}

static
const char* get_var_name(const char *name) {
    if (name && !ecs_os_strcmp(name, "This")) {
        /* Make sure that both This and . resolve to the same variable */
        name = ".";
    }

    return name;
}

static
ecs_rule_var_t* create_variable(
    ecs_rule_t *rule,
    ecs_rule_var_kind_t kind,
    const char *name)
{
    int32_t cur = ++ rule->variable_count;
    rule->variables = ecs_os_realloc(
        rule->variables, cur * ECS_SIZEOF(ecs_rule_var_t));

    name = get_var_name(name);

    ecs_rule_var_t *var = &rule->variables[cur - 1];
    if (name) {
        var->name = ecs_os_strdup(name);
    } else {
        /* Anonymous register */
        char name_buff[32];
        ecs_os_sprintf(name_buff, "_%u", cur - 1);
        var->name = ecs_os_strdup(name_buff);
    }

    var->kind = kind;

    /* The variable id is the location in the variable array and also points to
     * the register element that corresponds with the variable. */
    var->id = cur - 1;

    /* Depth is used to calculate how far the variable is from the root, where
     * the root is the variable with 0 dependencies. */
    var->depth = UINT8_MAX;
    var->marked = false;
    var->occurs = 0;

    return var;
}

static
ecs_rule_var_t* create_anonymous_variable(
    ecs_rule_t *rule,
    ecs_rule_var_kind_t kind)
{
    return create_variable(rule, kind, NULL);
}

/* Find variable with specified name and type. If Unknown is provided as type,
 * the function will return any variable with the provided name. The root 
 * variable can occur both as a table and entity variable, as some rules
 * require that each entity in a table is iterated. In this case, there are two
 * variables, one for the table and one for the entities in the table, that both
 * have the same name. */
static
ecs_rule_var_t* find_variable(
    const ecs_rule_t *rule,
    ecs_rule_var_kind_t kind,
    const char *name)
{
    ecs_assert(rule != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(name != NULL, ECS_INTERNAL_ERROR, NULL);

    name = get_var_name(name);

    ecs_rule_var_t *variables = rule->variables;
    int32_t i, count = rule->variable_count;
    
    for (i = 0; i < count; i ++) {
        ecs_rule_var_t *variable = &variables[i];
        if (!ecs_os_strcmp(name, variable->name)) {
            if (kind == EcsRuleVarKindUnknown || kind == variable->kind) {
                return variable;
            }
        }
    }

    return NULL;
}

/* Ensure variable with specified name and type exists. If an existing variable
 * is found with an unknown type, its type will be overwritten with the 
 * specified type. During the variable ordering phase it is not yet clear which
 * variable is the root. Which variable is the root determines its type, which
 * is why during this phase variables are still untyped. */
static
ecs_rule_var_t* ensure_variable(
    ecs_rule_t *rule,
    ecs_rule_var_kind_t kind,
    const char *name)
{
    ecs_rule_var_t *var = find_variable(rule, kind, name);
    if (!var) {
        var = create_variable(rule, kind, name);
    } else {
        if (var->kind == EcsRuleVarKindUnknown) {
            var->kind = kind;
        }
    }

    return var;
}

static
bool term_id_is_variable(
    ecs_term_id_t *term_id)
{
    return term_id->var == EcsVarIsVariable;
}

static
const char *term_id_var_name(
    ecs_term_id_t *term_id)
{
    if (term_id->var == EcsVarIsVariable) {
        if (term_id->entity == EcsThis) {
            return ".";
        } else {
            ecs_check(term_id->name != NULL, ECS_INVALID_PARAMETER, NULL);
            return term_id->name;
        }
    }
    
error:
    return NULL;
}

/* Get variable from a term identifier */
static
ecs_rule_var_t* term_id_to_var(
    ecs_rule_t *rule,
    ecs_term_id_t *id)
{
    if (id->var == EcsVarIsVariable) {;
        return find_variable(rule, EcsRuleVarKindUnknown, term_id_var_name(id));
    }
    return NULL;
}

/* Get variable from a term predicate */
static
ecs_rule_var_t* term_pred(
    ecs_rule_t *rule,
    ecs_term_t *term)
{
    return term_id_to_var(rule, &term->pred);
}

/* Get variable from a term subject */
static
ecs_rule_var_t* term_subj(
    ecs_rule_t *rule,
    ecs_term_t *term)
{
    return term_id_to_var(rule, &term->subj);
}

/* Get variable from a term object */
static
ecs_rule_var_t* term_obj(
    ecs_rule_t *rule,
    ecs_term_t *term)
{
    if (obj_is_set(term)) {
        return term_id_to_var(rule, &term->obj);
    } else {
        return NULL;
    }
}

/* Return predicate variable from pair */
static
ecs_rule_var_t* pair_pred(
    ecs_rule_t *rule,
    const ecs_rule_pair_t *pair)
{
    if (pair->reg_mask & RULE_PAIR_PREDICATE) {
        return &rule->variables[pair->pred.reg];
    } else {
        return NULL;
    }
}

/* Return object variable from pair */
static
ecs_rule_var_t* pair_obj(
    ecs_rule_t *rule,
    const ecs_rule_pair_t *pair)
{
    if (pair->reg_mask & RULE_PAIR_OBJECT) {
        return &rule->variables[pair->obj.reg];
    } else {
        return NULL;
    }
}

/* Create new frame for storing register values. Each operation that yields data
 * gets its own register frame, which contains all variables reified up to that
 * point. The preceding frame always contains the reified variables from the
 * previous operation. Operations that do not yield data (such as control flow)
 * do not have their own frames. */
static
int32_t push_frame(
    ecs_rule_t *rule)
{
    return rule->frame_count ++;
}

/* Get register array for current stack frame. The stack frame is determined by
 * the current operation that is evaluated. The register array contains the
 * values for the reified variables. If a variable hasn't been reified yet, its
 * register will store a wildcard. */
static
ecs_rule_reg_t* get_register_frame(
    ecs_rule_iter_t *it,
    int32_t frame)    
{
    if (it->registers) {
        return &it->registers[frame * it->rule->variable_count];
    } else {
        return NULL;
    }
}

/* Get register array for current stack frame. The stack frame is determined by
 * the current operation that is evaluated. The register array contains the
 * values for the reified variables. If a variable hasn't been reified yet, its
 * register will store a wildcard. */
static
ecs_rule_reg_t* get_registers(
    ecs_rule_iter_t *it,
    ecs_rule_op_t *op)    
{
    return get_register_frame(it, op->frame);
}

/* Get columns array. Columns store, for each matched column in a table, the 
 * index at which it occurs. This reduces the amount of searching that
 * operations need to do in a type, since select/with already provide it. */
static
int32_t* rule_get_columns_frame(
    ecs_rule_iter_t *it,
    int32_t frame)    
{
    return &it->columns[frame * it->rule->filter.term_count];
}

static
int32_t* rule_get_columns(
    ecs_rule_iter_t *it,
    ecs_rule_op_t *op)    
{
    return rule_get_columns_frame(it, op->frame);
}

static
ecs_table_t* table_from_entity(
    ecs_world_t *world,
    ecs_entity_t e)
{
    ecs_record_t *record = ecs_eis_get(world, e);
    if (record) {
        return record->table;
    } else {
        return NULL;
    }
}

static
void entity_reg_set(
    const ecs_rule_t *rule,
    ecs_rule_reg_t *regs,
    int32_t r,
    ecs_entity_t entity)
{
    (void)rule;
    ecs_assert(rule->variables[r].kind == EcsRuleVarKindEntity, 
        ECS_INTERNAL_ERROR, NULL);
    ecs_check(ecs_is_valid(rule->world, entity), ECS_INVALID_PARAMETER, NULL);
    regs[r].entity = entity;
error:
    return;
}

static
ecs_entity_t entity_reg_get(
    const ecs_rule_t *rule,
    ecs_rule_reg_t *regs,
    int32_t r)
{
    (void)rule;
    ecs_entity_t e = regs[r].entity;
    if (!e) {
        return EcsWildcard;
    }
    
    ecs_check(ecs_is_valid(rule->world, e), ECS_INVALID_PARAMETER, NULL);   
    return e;
error:
    return 0;
}

static
void table_reg_set(
    const ecs_rule_t *rule,
    ecs_rule_reg_t *regs,
    int32_t r,
    ecs_table_t *table)
{
    (void)rule;
    ecs_assert(rule->variables[r].kind == EcsRuleVarKindTable, 
        ECS_INTERNAL_ERROR, NULL);

    regs[r].table = table;
    regs[r].offset = 0;
    regs[r].count = 0;
    regs[r].entity = 0;
}

static 
ecs_table_t* table_reg_get(
    const ecs_rule_t *rule,
    ecs_rule_reg_t *regs,
    int32_t r)
{
    (void)rule;
    ecs_assert(rule->variables[r].kind == EcsRuleVarKindTable, 
        ECS_INTERNAL_ERROR, NULL);

    return regs[r].table;       
}

static
ecs_entity_t reg_get_entity(
    const ecs_rule_t *rule,
    ecs_rule_op_t *op,
    ecs_rule_reg_t *regs,
    int32_t r)
{
    if (r == UINT8_MAX) {
        ecs_assert(op->subject != 0, ECS_INTERNAL_ERROR, NULL);

        /* The subject is referenced from the query string by string identifier.
         * If subject entity is not valid, it could have been deletd by the
         * application after the rule was created */
        ecs_check(ecs_is_valid(rule->world, op->subject), 
            ECS_INVALID_PARAMETER, NULL);

        return op->subject;
    }
    if (rule->variables[r].kind == EcsRuleVarKindTable) {
        int32_t offset = regs[r].offset;

        ecs_assert(regs[r].count == 1, ECS_INTERNAL_ERROR, NULL);
        ecs_data_t *data = &table_reg_get(rule, regs, r)->storage;
        ecs_assert(data != NULL, ECS_INTERNAL_ERROR, NULL);
        ecs_entity_t *entities = ecs_vector_first(data->entities, ecs_entity_t);
        ecs_assert(entities != NULL, ECS_INTERNAL_ERROR, NULL);
        ecs_assert(offset < ecs_vector_count(data->entities), 
            ECS_INTERNAL_ERROR, NULL);
        ecs_check(ecs_is_valid(rule->world, entities[offset]), 
            ECS_INVALID_PARAMETER, NULL);            
        
        return entities[offset];
    }
    if (rule->variables[r].kind == EcsRuleVarKindEntity) {
        return entity_reg_get(rule, regs, r);
    }

    /* Must return an entity */
    ecs_assert(false, ECS_INTERNAL_ERROR, NULL);

error:
    return 0;
}

static
ecs_table_t* reg_get_table(
    const ecs_rule_t *rule,
    ecs_rule_op_t *op,
    ecs_rule_reg_t *regs,
    int32_t r)
{
    if (r == UINT8_MAX) {
        ecs_assert(op->subject != 0, ECS_INTERNAL_ERROR, NULL);
        ecs_check(ecs_is_valid(rule->world, op->subject), 
            ECS_INVALID_PARAMETER, NULL);

        return table_from_entity(rule->world, op->subject);
    }
    if (rule->variables[r].kind == EcsRuleVarKindTable) {
        return table_reg_get(rule, regs, r);
    }
    if (rule->variables[r].kind == EcsRuleVarKindEntity) {
        return table_from_entity(rule->world, entity_reg_get(rule, regs, r));
    } 
error:
    return NULL;
}

static
void reg_set_entity(
    const ecs_rule_t *rule,
    ecs_rule_reg_t *regs,
    int32_t r,
    ecs_entity_t entity)
{
    if (rule->variables[r].kind == EcsRuleVarKindTable) {
        ecs_world_t *world = rule->world;
        ecs_check(ecs_is_valid(world, entity), ECS_INVALID_PARAMETER, NULL);

        ecs_record_t *record = ecs_eis_get(world, entity);
        if (!record || !record->table) {
            regs[r].table = NULL;
            regs[r].offset = 0;
            regs[r].count = 0;
            regs[r].entity = entity;
        } else {
            regs[r].table = record->table;
            regs[r].offset = ECS_RECORD_TO_ROW(record->row);
            regs[r].count = 1;
            regs[r].entity = 0;
        }
    } else {
        entity_reg_set(rule, regs, r, entity);
    }
error:
    return;
}

/* This encodes a column expression into a pair. A pair stores information about
 * the variable(s) associated with the column. Pairs are used by operations to
 * apply filters, and when there is a match, to reify variables. */
static
ecs_rule_pair_t term_to_pair(
    ecs_rule_t *rule,
    ecs_term_t *term)
{
    ecs_rule_pair_t result = {0};

    /* Terms must always have at least one argument (the subject) */
    ecs_assert(subj_is_set(term), ECS_INTERNAL_ERROR, NULL);

    /* If the predicate id is a variable, find the variable and encode its id
     * in the pair so the operation can find it later. */
    if (term->pred.var == EcsVarIsVariable) {
        /* Always lookup the as an entity, as pairs never refer to tables */
        const ecs_rule_var_t *var = find_variable(
            rule, EcsRuleVarKindEntity, term_id_var_name(&term->pred));

        /* Variables should have been declared */
        ecs_assert(var != NULL, ECS_INTERNAL_ERROR, NULL);
        ecs_assert(var->kind == EcsRuleVarKindEntity, ECS_INTERNAL_ERROR, NULL);
        result.pred.reg = var->id;

        /* Set flag so the operation can see that the predicate is a variable */
        result.reg_mask |= RULE_PAIR_PREDICATE;
        result.final = true;
    } else {
        /* If the predicate is not a variable, simply store its id. */
        ecs_entity_t pred_id = term->pred.entity;
        result.pred.ent = pred_id;

        /* Test if predicate is transitive. When evaluating the predicate, this
         * will also take into account transitive relationships */
        if (ecs_has_id(rule->world, pred_id, EcsTransitive)) {
            /* Transitive queries must have an object */
            if (obj_is_set(term)) {
                result.transitive = true;
            }
        }

        if (ecs_has_id(rule->world, pred_id, EcsFinal)) {
            result.final = true;
        }

        if (ecs_has_id(rule->world, pred_id, EcsTransitiveSelf)) {
            result.inclusive = true;
        }
    }

    /* The pair doesn't do anything with the subject (subjects are the things that
     * are matched against pairs) so if the column does not have a object, 
     * there is nothing left to do. */
    if (!obj_is_set(term)) {
        return result;
    }

    /* If arguments is higher than 2 this is not a pair but a nested rule */
    ecs_assert(obj_is_set(term), ECS_INTERNAL_ERROR, NULL);

    /* Same as above, if the object is a variable, store it and flag it */
    if (term->obj.var == EcsVarIsVariable) {
        const ecs_rule_var_t *var = find_variable(
            rule, EcsRuleVarKindEntity, term_id_var_name(&term->obj));

        /* Variables should have been declared */
        ecs_assert(var != NULL, ECS_INTERNAL_ERROR, NULL);
        ecs_assert(var->kind == EcsRuleVarKindEntity, ECS_INTERNAL_ERROR, NULL);

        result.obj.reg = var->id;
        result.reg_mask |= RULE_PAIR_OBJECT;
    } else {
        /* If the object is not a variable, simply store its id */
        result.obj.ent = term->obj.entity;
        if (!result.obj.ent) {
            result.obj_0 = true;
        }
    }

    return result;
}

/* When an operation has a pair, it is used to filter its input. This function
 * translates a pair back into an entity id, and in the process substitutes the
 * variables that have already been filled out. It's one of the most important
 * functions, as a lot of the filtering logic depends on having an entity that
 * has all of the reified variables correctly filled out. */
static
ecs_rule_filter_t pair_to_filter(
    ecs_rule_iter_t *it,
    ecs_rule_op_t *op,
    ecs_rule_pair_t pair)
{
    ecs_entity_t pred = pair.pred.ent;
    ecs_entity_t obj = pair.obj.ent;
    ecs_rule_filter_t result = {
        .lo_var = -1,
        .hi_var = -1
    };

    /* Get registers in case we need to resolve ids from registers. Get them
     * from the previous, not the current stack frame as the current operation
     * hasn't reified its variables yet. */
    ecs_rule_reg_t *regs = get_register_frame(it, op->frame - 1);

    if (pair.reg_mask & RULE_PAIR_OBJECT) {
        obj = entity_reg_get(it->rule, regs, pair.obj.reg);
        obj = ecs_entity_t_lo(obj); /* Filters don't have generations */

        if (obj == EcsWildcard) {
            result.wildcard = true;
            result.obj_wildcard = true;
            result.lo_var = pair.obj.reg;
        }
    }

    if (pair.reg_mask & RULE_PAIR_PREDICATE) {
        pred = entity_reg_get(it->rule, regs, pair.pred.reg);
        pred = ecs_entity_t_lo(pred); /* Filters don't have generations */

        if (pred == EcsWildcard) {
            if (result.wildcard) {
                result.same_var = pair.pred.reg == pair.obj.reg;
            }

            result.wildcard = true;
            result.pred_wildcard = true;

            if (obj) {
                result.hi_var = pair.pred.reg;
            } else {
                result.lo_var = pair.pred.reg;
            }
        }
    }

    if (!obj && !pair.obj_0) {
        result.mask = pred;
    } else {
        result.mask = ecs_pair(pred, obj);
    }

    return result;
}

/* This function is responsible for reifying the variables (filling them out 
 * with their actual values as soon as they are known). It uses the pair 
 * expression returned by pair_get_most_specific_var, and attempts to fill out each of the
 * wildcards in the pair. If a variable isn't reified yet, the pair expression
 * will still contain one or more wildcards, which is harmless as the respective
 * registers will also point to a wildcard. */
static
void reify_variables(
    ecs_rule_iter_t *it,
    ecs_rule_op_t *op,
    ecs_rule_filter_t *filter,
    ecs_type_t type,
    int32_t column)
{
    const ecs_rule_t *rule = it->rule;
    const ecs_rule_var_t *vars = rule->variables;
    (void)vars;

    ecs_rule_reg_t *regs = get_registers(it, op);
    ecs_entity_t *elem = ecs_vector_get(type, ecs_entity_t, column);
    ecs_assert(elem != NULL, ECS_INTERNAL_ERROR, NULL);

    int32_t obj_var = filter->lo_var;
    int32_t pred_var = filter->hi_var;

    if (obj_var != -1) {
        ecs_assert(vars[obj_var].kind == EcsRuleVarKindEntity, 
            ECS_INTERNAL_ERROR, NULL);

        entity_reg_set(rule, regs, obj_var, 
            ecs_get_alive(rule->world, ECS_PAIR_OBJECT(*elem)));
    }

    if (pred_var != -1) {
        ecs_assert(vars[pred_var].kind == EcsRuleVarKindEntity, 
            ECS_INTERNAL_ERROR, NULL);            

        entity_reg_set(rule, regs, pred_var, 
            ecs_get_alive(rule->world, 
                ECS_PAIR_RELATION(*elem)));
    }
}

/* Returns whether variable is a subject */
static
bool is_subject(
    ecs_rule_t *rule,
    ecs_rule_var_t *var)
{
    ecs_assert(rule != NULL, ECS_INTERNAL_ERROR, NULL);

    if (!var) {
        return false;
    }

    if (var->id < rule->subject_variable_count) {
        return true;
    }

    return false;
}

static
bool skip_term(ecs_term_t *term) {
    if (term->subj.set.mask & EcsNothing) {
        return true;
    }
    if (term->oper == EcsNot) {
        return true;
    }

    return false;
}

static
int32_t get_variable_depth(
    ecs_rule_t *rule,
    ecs_rule_var_t *var,
    ecs_rule_var_t *root,
    int recur);

static
int32_t crawl_variable(
    ecs_rule_t *rule,
    ecs_rule_var_t *var,
    ecs_rule_var_t *root,
    int recur)    
{
    ecs_term_t *terms = rule->filter.terms;
    int32_t i, count = rule->filter.term_count;

    for (i = 0; i < count; i ++) {
        ecs_term_t *term = &terms[i];
        if (skip_term(term)) {
            continue;
        }
        
        ecs_rule_var_t 
        *pred = term_pred(rule, term),
        *subj = term_subj(rule, term),
        *obj = term_obj(rule, term);

        /* Variable must at least appear once in term */
        if (var != pred && var != subj && var != obj) {
            continue;
        }

        if (pred && pred != var && !pred->marked) {
            get_variable_depth(rule, pred, root, recur + 1);
        }

        if (subj && subj != var && !subj->marked) {
            get_variable_depth(rule, subj, root, recur + 1);
        }

        if (obj && obj != var && !obj->marked) {
            get_variable_depth(rule, obj, root, recur + 1);
        }
    }

    return 0;
}

static
int32_t get_depth_from_var(
    ecs_rule_t *rule,
    ecs_rule_var_t *var,
    ecs_rule_var_t *root,
    int recur)
{
    /* If variable is the root or if depth has been set, return depth + 1 */
    if (var == root || var->depth != UINT8_MAX) {
        return var->depth + 1;
    }

    /* Variable is already being evaluated, so this indicates a cycle. Stop */
    if (var->marked) {
        return 0;
    }
    
    /* Variable is not yet being evaluated and depth has not yet been set. 
     * Calculate depth. */
    int32_t depth = get_variable_depth(rule, var, root, recur + 1);
    if (depth == UINT8_MAX) {
        return depth;
    } else {
        return depth + 1;
    }
}

static
int32_t get_depth_from_term(
    ecs_rule_t *rule,
    ecs_rule_var_t *cur,
    ecs_rule_var_t *pred,
    ecs_rule_var_t *obj,
    ecs_rule_var_t *root,
    int recur)
{
    int32_t result = UINT8_MAX;

    /* If neither of the other parts of the terms are variables, this
     * variable is guaranteed to have no dependencies. */
    if (!pred && !obj) {
        result = 0;
    } else {
        /* If this is a variable that is not the same as the current, 
         * we can use it to determine dependency depth. */
        if (pred && cur != pred) {
            int32_t depth = get_depth_from_var(rule, pred, root, recur);
            if (depth == UINT8_MAX) {
                return UINT8_MAX;
            }

            /* If the found depth is lower than the depth found, overwrite it */
            if (depth < result) {
                result = depth;
            }
        }

        /* Same for obj */
        if (obj && cur != obj) {
            int32_t depth = get_depth_from_var(rule, obj, root, recur);
            if (depth == UINT8_MAX) {
                return UINT8_MAX;
            }

            if (depth < result) {
                result = depth;
            }
        }
    }

    return result;
}

/* Find the depth of the dependency tree from the variable to the root */
static
int32_t get_variable_depth(
    ecs_rule_t *rule,
    ecs_rule_var_t *var,
    ecs_rule_var_t *root,
    int recur)
{
    var->marked = true;

    /* Iterate columns, find all instances where 'var' is not used as subject.
     * If the subject of that column is either the root or a variable for which
     * the depth is known, the depth for this variable can be determined. */
    ecs_term_t *terms = rule->filter.terms;

    int32_t i, count = rule->filter.term_count;
    int32_t result = UINT8_MAX;

    for (i = 0; i < count; i ++) {
        ecs_term_t *term = &terms[i];
        if (skip_term(term)) {
            continue;
        }

        ecs_rule_var_t 
        *pred = term_pred(rule, term),
        *subj = term_subj(rule, term),
        *obj = term_obj(rule, term);

        if (subj != var) {
            continue;
        }

        if (!is_subject(rule, pred)) {
            pred = NULL;
        }

        if (!is_subject(rule, obj)) {
            obj = NULL;
        }

        int32_t depth = get_depth_from_term(rule, var, pred, obj, root, recur);
        if (depth < result) {
            result = depth;
        }
    }

    if (result == UINT8_MAX) {
        result = 0;
    }

    var->depth = result;    

    /* Dependencies are calculated from subject to (pred, obj). If there were
     * subjects that are only related by object (like (X, Y), (Z, Y)) it is
     * possible that those have not yet been found yet. To make sure those 
     * variables are found, loop again & follow predicate & object links */
    for (i = 0; i < count; i ++) {
        ecs_term_t *term = &terms[i];
        if (skip_term(term)) {
            continue;
        }

        ecs_rule_var_t 
        *subj = term_subj(rule, term),
        *pred = term_pred(rule, term),
        *obj = term_obj(rule, term);

        /* Only evaluate pred & obj for current subject. This ensures that we
         * won't evaluate variables that are unreachable from the root. This
         * must be detected as unconstrained variables are not allowed. */
        if (subj != var) {
            continue;
        }

        crawl_variable(rule, subj, root, recur);

        if (pred && pred != var) {
            crawl_variable(rule, pred, root, recur);
        }

        if (obj && obj != var) {
            crawl_variable(rule, obj, root, recur);
        }        
    }

    return var->depth;
}

/* Compare function used for qsort. It ensures that variables are first ordered
 * by depth, followed by how often they occur. */
static
int compare_variable(
    const void* ptr1, 
    const void *ptr2)
{
    const ecs_rule_var_t *v1 = ptr1;
    const ecs_rule_var_t *v2 = ptr2;

    if (v1->kind < v2->kind) {
        return -1;
    } else if (v1->kind > v2->kind) {
        return 1;
    }

    if (v1->depth < v2->depth) {
        return -1;
    } else if (v1->depth > v2->depth) {
        return 1;
    }

    if (v1->occurs < v2->occurs) {
        return 1;
    } else {
        return -1;
    }

    return (v1->id < v2->id) - (v1->id > v2->id);
}

/* After all subject variables have been found, inserted and sorted, the 
 * remaining variables (predicate & object) still need to be inserted. This
 * function serves two purposes. The first purpose is to ensure that all 
 * variables are known before operations are emitted. This ensures that the
 * variables array won't be reallocated while emitting, which simplifies code.
 * The second purpose of the function is to ensure that if the root variable
 * (which, if it exists has now been created with a table type) is also inserted
 * with an entity type if required. This is used later to decide whether the
 * rule needs to insert an each instruction. */
static
void ensure_all_variables(
    ecs_rule_t *rule)
{
    ecs_term_t *terms = rule->filter.terms;
    int32_t i, count = rule->filter.term_count;

    for (i = 0; i < count; i ++) {
        ecs_term_t *term = &terms[i];
        if (skip_term(term)) {
            continue;
        }

        /* If predicate is a variable, make sure it has been registered */
        if (term->pred.var == EcsVarIsVariable) {
            ensure_variable(rule, EcsRuleVarKindEntity, term_id_var_name(&term->pred));
        }

        /* If subject is a variable and it is not This, make sure it is 
         * registered as an entity variable. This ensures that the program will
         * correctly return all permutations */
        if (term->subj.var == EcsVarIsVariable) {
            if (term->subj.entity != EcsThis) {
                ensure_variable(rule, EcsRuleVarKindEntity, term_id_var_name(&term->subj));
            }
        }

        /* If object is a variable, make sure it has been registered */
        if (obj_is_set(term) && (term->obj.var == EcsVarIsVariable)) {
            ensure_variable(rule, EcsRuleVarKindEntity, term_id_var_name(&term->obj));
        }
    }    
}

/* Scan for variables, put them in optimal dependency order. */
static
int scan_variables(
    ecs_rule_t *rule)
{
    /* Objects found in rule. One will be elected root */
    int32_t subject_count = 0;

    /* If this (.) is found, it always takes precedence in root election */
    int32_t this_var = UINT8_MAX;

    /* Keep track of the subject variable that occurs the most. In the absence of
     * this (.) the variable with the most occurrences will be elected root. */
    int32_t max_occur = 0;
    int32_t max_occur_var = UINT8_MAX;

    /* Step 1: find all possible roots */
    ecs_term_t *terms = rule->filter.terms;
    int32_t i, term_count = rule->filter.term_count;

    for (i = 0; i < term_count; i ++) {
        ecs_term_t *term = &terms[i];

        /* Evaluate the subject. The predicate and object are not evaluated, 
         * since they never can be elected as root. */
        if (term_id_is_variable(&term->subj)) {
            const char *subj_name = term_id_var_name(&term->subj);
            
            ecs_rule_var_t *subj = find_variable(
                rule, EcsRuleVarKindTable, subj_name);
            if (!subj) {
                subj = create_variable(rule, EcsRuleVarKindTable, subj_name);
                if (subject_count >= ECS_RULE_MAX_VARIABLE_COUNT) {
                    rule_error(rule, "too many variables in rule");
                    goto error;
                }
            }

            if (++ subj->occurs > max_occur) {
                max_occur = subj->occurs;
                max_occur_var = subj->id;
            }
        }
    }

    rule->subject_variable_count = rule->variable_count;

    ensure_all_variables(rule);

    /* Variables in a term with a literal subject have depth 0 */
    for (i = 0; i < term_count; i ++) {
        ecs_term_t *term = &terms[i];

        if (term->subj.var == EcsVarIsEntity) {
            ecs_rule_var_t 
            *pred = term_pred(rule, term),
            *obj = term_obj(rule, term);

            if (pred) {
                pred->depth = 0;
            }
            if (obj) {
                obj->depth = 0;
            }
        }
    }

    /* Elect a root. This is either this (.) or the variable with the most
     * occurrences. */
    int32_t root_var = this_var;
    if (root_var == UINT8_MAX) {
        root_var = max_occur_var;
        if (root_var == UINT8_MAX) {
            /* If no subject variables have been found, the rule expression only
             * operates on a fixed set of entities, in which case no root 
             * election is required. */
            goto done;
        }
    }

    ecs_rule_var_t *root = &rule->variables[root_var];
    root->depth = get_variable_depth(rule, root, root, 0);

    /* Verify that there are no unconstrained variables. Unconstrained variables
     * are variables that are unreachable from the root. */
    for (i = 0; i < rule->subject_variable_count; i ++) {
        if (rule->variables[i].depth == UINT8_MAX) {
            rule_error(rule, "unconstrained variable '%s'", 
                rule->variables[i].name);
            goto error;
        } 
    }

    /* For each Not term, verify that variables are known */
    for (i = 0; i < term_count; i ++) {
        ecs_term_t *term = &terms[i];
        if (term->oper != EcsNot) {
            continue;
        }

        ecs_rule_var_t 
        *pred = term_pred(rule, term),
        *obj = term_obj(rule, term);

        if (!pred && term_id_is_variable(&term->pred)) {
            rule_error(rule, "missing predicate variable '%s'", 
                term_id_var_name(&term->pred));
            goto error;
        }
        if (!obj && term_id_is_variable(&term->obj)) {
            rule_error(rule, "missing object variable '%s'", 
                term_id_var_name(&term->obj));
            goto error;
        }
    }

    /* Order variables by depth, followed by occurrence. The variable
     * array will later be used to lead the iteration over the terms, and
     * determine which operations get inserted first. */
    int32_t var_count = rule->variable_count;
    ecs_qsort_t(rule->variables, var_count, ecs_rule_var_t, compare_variable);

    /* Iterate variables to correct ids after sort */
    for (i = 0; i < rule->variable_count; i ++) {
        rule->variables[i].id = i;
    }
    
done:
    return 0;
error:
    return -1;
}

/* Get entity variable from table variable */
static
ecs_rule_var_t* to_entity(
    ecs_rule_t *rule,
    ecs_rule_var_t *var)
{
    if (!var) {
        return NULL;
    }

    ecs_rule_var_t *evar = NULL;
    if (var->kind == EcsRuleVarKindTable) {
        evar = find_variable(rule, EcsRuleVarKindEntity, var->name);
    } else {
        evar = var;
    }

    return evar;
}

/* Ensure that if a table variable has been written, the corresponding entity
 * variable is populated. The function will return the most specific, populated
 * variable. */
static
ecs_rule_var_t* most_specific_var(
    ecs_rule_t *rule,
    ecs_rule_var_t *var,
    bool *written,
    bool create)
{
    if (!var) {
        return NULL;
    }

    ecs_rule_var_t *tvar, *evar = to_entity(rule, var);
    if (!evar) {
        return var;
    }

    if (var->kind == EcsRuleVarKindTable) {
        tvar = var;
    } else {
        tvar = find_variable(rule, EcsRuleVarKindTable, var->name);
    }

    /* If variable is used as predicate or object, it should have been 
     * registered as an entity. */
    ecs_assert(evar != NULL, ECS_INTERNAL_ERROR, NULL);

    /* Usually table variables are resolved before they are used as a predicate
     * or object, but in the case of cyclic dependencies this is not guaranteed.
     * Only insert an each instruction of the table variable has been written */
    if (tvar && written[tvar->id]) {
        /* If the variable has been written as a table but not yet
         * as an entity, insert an each operation that yields each
         * entity in the table. */
        if (evar) {
            if (written[evar->id]) {
                return evar;
            } else if (create) {
                ecs_rule_op_t *op = create_operation(rule);
                op->kind = EcsRuleEach;
                op->on_pass = rule->operation_count;
                op->on_fail = rule->operation_count - 2;
                op->frame = rule->frame_count;
                op->has_in = true;
                op->has_out = true;
                op->r_in = tvar->id;
                op->r_out = evar->id;

                /* Entity will either be written or has been written */
                written[evar->id] = true;

                push_frame(rule);

                return evar;
            } else {
                return tvar;
            }
        }
    } else if (evar && written[evar->id]) {
        return evar;
    }

    return var;
}

/* Get most specific known variable */
static
ecs_rule_var_t *get_most_specific_var(
    ecs_rule_t *rule,
    ecs_rule_var_t *var,
    bool *written)
{
    return most_specific_var(rule, var, written, false);
}

/* Get or create most specific known variable. This will populate an entity
 * variable if a table variable is known but the entity variable isn't. */
static
ecs_rule_var_t *ensure_most_specific_var(
    ecs_rule_t *rule,
    ecs_rule_var_t *var,
    bool *written)
{
    return most_specific_var(rule, var, written, true);
}


/* Ensure that an entity variable is written before using it */
static
ecs_rule_var_t* ensure_entity_written(
    ecs_rule_t *rule,
    ecs_rule_var_t *var,
    bool *written)
{
    if (!var) {
        return NULL;
    }

    /* Ensure we're working with the most specific version of subj we can get */
    ecs_rule_var_t *evar = ensure_most_specific_var(rule, var, written);

    /* The post condition of this function is that there is an entity variable,
     * and that it is written. Make sure that the result is an entity */
    ecs_assert(evar != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(evar->kind == EcsRuleVarKindEntity, ECS_INTERNAL_ERROR, NULL);

    /* Make sure the variable has been written */
    ecs_assert(written[evar->id] == true, ECS_INTERNAL_ERROR, NULL);
    
    return evar;
}

static
ecs_rule_op_t* insert_operation(
    ecs_rule_t *rule,
    int32_t term_index,
    bool *written)
{
    ecs_rule_pair_t pair = {0};

    /* Parse the term's type into a pair. A pair extracts the ids from
     * the term, and replaces variables with wildcards which can then
     * be matched against actual relationships. A pair retains the 
     * information about the variables, so that when a match happens,
     * the pair can be used to reify the variable. */
    if (term_index != -1) {
        ecs_term_t *term = &rule->filter.terms[term_index];

        pair = term_to_pair(rule, term);

        /* If the pair contains entity variables that have not yet been written,
         * insert each instructions in case their tables are known. Variables in
         * a pair that are truly unknown will be populated by the operation,
         * but an operation should never overwrite an entity variable if the 
         * corresponding table variable has already been resolved. */
        if (pair.reg_mask & RULE_PAIR_PREDICATE) {
            ecs_rule_var_t *pred = &rule->variables[pair.pred.reg];
            pred = get_most_specific_var(rule, pred, written);
            pair.pred.reg = pred->id;
        }

        if (pair.reg_mask & RULE_PAIR_OBJECT) {
            ecs_rule_var_t *obj = &rule->variables[pair.obj.reg];
            obj = get_most_specific_var(rule, obj, written);
            pair.obj.reg = obj->id;
        }
    } else {
        /* Not all operations have a filter (like Each) */
    }

    ecs_rule_op_t *op = create_operation(rule);
    op->on_pass = rule->operation_count;
    op->on_fail = rule->operation_count - 2;
    op->frame = rule->frame_count;
    op->filter = pair;

    /* Store corresponding signature term so we can correlate and
     * store the table columns with signature columns. */
    op->term = term_index;

    return op;
}

/* Insert first operation, which is always Input. This creates an entry in
 * the register stack for the initial state. */
static
void insert_input(
    ecs_rule_t *rule)
{
    ecs_rule_op_t *op = create_operation(rule);
    op->kind = EcsRuleInput;

    /* The first time Input is evaluated it goes to the next/first operation */
    op->on_pass = 1;

    /* When Input is evaluated with redo = true it will return false, which will
     * finish the program as op becomes -1. */
    op->on_fail = -1;  

    push_frame(rule);
}

/* Insert last operation, which is always Yield. When the program hits Yield,
 * data is returned to the application. */
static
void insert_yield(
    ecs_rule_t *rule)
{
    ecs_rule_op_t *op = create_operation(rule);
    op->kind = EcsRuleYield;
    op->has_in = true;
    op->on_fail = rule->operation_count - 2;
    /* Yield can only "fail" since it is the end of the program */

    /* Find variable associated with this. It is possible that the variable
     * exists both as a table and as an entity. This can happen when a rule
     * first selects a table for this, but then subsequently needs to evaluate
     * each entity in that table. In that case the yield instruction should
     * return the entity, so look for that first. */
    ecs_rule_var_t *var = find_variable(rule, EcsRuleVarKindEntity, ".");
    if (!var) {
        var = find_variable(rule, EcsRuleVarKindTable, ".");
    }

    /* If there is no this, there is nothing to yield. In that case the rule
     * simply returns true or false. */
    if (!var) {
        op->r_in = UINT8_MAX;
    } else {
        op->r_in = var->id;
    }

    op->frame = push_frame(rule);
}

/* Return superset/subset including the root */
static
void insert_inclusive_set(
    ecs_rule_t *rule,
    ecs_rule_op_kind_t op_kind,
    ecs_rule_var_t *out,
    const ecs_rule_pair_t pair,
    int32_t c,
    bool *written,
    bool inclusive)
{
    ecs_assert(out != NULL, ECS_INTERNAL_ERROR, NULL);

    /* If the operation to be inserted is a superset, the output variable needs
     * to be an entity as a superset is always resolved one at a time */
    ecs_assert((op_kind != EcsRuleSuperSet) || 
        out->kind == EcsRuleVarKindEntity, ECS_INTERNAL_ERROR, NULL);

    int32_t setjmp_lbl = rule->operation_count;
    int32_t store_lbl = setjmp_lbl + 1;
    int32_t set_lbl = setjmp_lbl + 2;
    int32_t next_op = setjmp_lbl + 4;
    int32_t prev_op = setjmp_lbl - 1;

    /* Insert 4 operations at once, so we don't have to worry about how
     * the instruction array reallocs. If operation is not inclusive, we only
     * need to insert the set operation. */
    if (inclusive) {
        insert_operation(rule, -1, written);
        insert_operation(rule, -1, written);
        insert_operation(rule, -1, written);
    }

    ecs_rule_op_t *op = insert_operation(rule, -1, written);
    ecs_rule_op_t *setjmp = op - 3;
    ecs_rule_op_t *store = op - 2;
    ecs_rule_op_t *set = op - 1;
    ecs_rule_op_t *jump = op;

    if (!inclusive) {
        set_lbl = setjmp_lbl;
        set = op;
        setjmp = NULL;
        store = NULL;
        jump = NULL;
        next_op = set_lbl + 1;
        prev_op = set_lbl - 1;
    }

    /* The SetJmp operation stores a conditional jump label that either
     * points to the Store or *Set operation */
    if (inclusive) {
        setjmp->kind = EcsRuleSetJmp;
        setjmp->on_pass = store_lbl;
        setjmp->on_fail = set_lbl;
    }

    ecs_rule_var_t *pred = pair_pred(rule, &pair);
    ecs_rule_var_t *obj = pair_obj(rule, &pair);

    /* The Store operation yields the root of the subtree. After yielding,
     * this operation will fail and return to SetJmp, which will cause it
     * to switch to the *Set operation. */
    if (inclusive) {
        store->kind = EcsRuleStore;
        store->on_pass = next_op;
        store->on_fail = setjmp_lbl;
        store->has_in = true;
        store->has_out = true;
        store->r_out = out->id;
        store->term = c;

        if (!pred) {
            store->filter.pred = pair.pred;
        } else {
            store->filter.pred.reg = pred->id;
            store->filter.reg_mask |= RULE_PAIR_PREDICATE;
        }

        /* If the object of the filter is not a variable, store literal */
        if (!obj) {
            store->r_in = UINT8_MAX;
            store->subject = ecs_get_alive(rule->world, pair.obj.ent);
            store->filter.obj = pair.obj;
        } else {
            store->r_in = obj->id;
            store->filter.obj.reg = obj->id;
            store->filter.reg_mask |= RULE_PAIR_OBJECT;
        }
    }

    /* This is either a SubSet or SuperSet operation */
    set->kind = op_kind;
    set->on_pass = next_op;
    set->on_fail = prev_op;
    set->has_out = true;
    set->r_out = out->id;
    set->term = c;

    /* Predicate can be a variable if it's non-final */
    if (!pred) {
        set->filter.pred = pair.pred;
    } else {
        set->filter.pred.reg = pred->id;
        set->filter.reg_mask |= RULE_PAIR_PREDICATE;
    }

    if (!obj) {
        set->filter.obj = pair.obj;
    } else {
        set->filter.obj.reg = obj->id;
        set->filter.reg_mask |= RULE_PAIR_OBJECT;
    }

    if (inclusive) {
        /* The jump operation jumps to either the store or subset operation,
        * depending on whether the store operation already yielded. The 
        * operation is inserted last, so that the on_fail label of the next 
        * operation will point to it */
        jump->kind = EcsRuleJump;
        
        /* The pass/fail labels of the Jump operation are not used, since it
        * jumps to a variable location. Instead, the pass label is (ab)used to
        * store the label of the SetJmp operation, so that the jump can access
        * the label it needs to jump to from the setjmp op_ctx. */
        jump->on_pass = setjmp_lbl;
        jump->on_fail = -1;
    }

    written[out->id] = true;
}

static
ecs_rule_var_t* store_inclusive_set(
    ecs_rule_t *rule,
    ecs_rule_op_kind_t op_kind,
    ecs_rule_pair_t *pair,
    bool *written,
    bool inclusive)
{
    /* The subset operation returns tables */
    ecs_rule_var_kind_t var_kind = EcsRuleVarKindTable;

    /* The superset operation returns entities */
    if (op_kind == EcsRuleSuperSet) {
        var_kind = EcsRuleVarKindEntity;   
    }

    /* Create anonymous variable for storing the set */
    ecs_rule_var_t *av = create_anonymous_variable(rule, var_kind);
    ecs_rule_var_t *ave = NULL;

    /* If the variable kind is a table, also create an entity variable as the
     * result of the set operation should be returned as an entity */
    if (var_kind == EcsRuleVarKindTable) {
        ave = create_variable(rule, EcsRuleVarKindEntity, av->name);
        av = &rule->variables[ave->id - 1];
    }

    /* Ensure we're using the most specific version of obj */
    ecs_rule_var_t *obj = pair_obj(rule, pair);
    if (obj) {
        pair->obj.reg = obj->id;
    }

    /* Generate the operations */
    insert_inclusive_set(rule, op_kind, av, *pair, -1, written, inclusive);

    /* Make sure to return entity variable, and that it is populated */
    return ensure_entity_written(rule, av, written);
}

static
bool is_known(
    ecs_rule_var_t *var,
    bool *written)
{
    if (!var) {
        return true;
    } else {
        return written[var->id];
    }
}

static
bool is_pair_known(
    ecs_rule_t *rule,
    ecs_rule_pair_t *pair,
    bool *written)
{
    ecs_rule_var_t *pred_var = pair_pred(rule, pair);
    if (!is_known(pred_var, written)) {
        return false;
    }

    ecs_rule_var_t *obj_var = pair_obj(rule, pair);
    if (!is_known(obj_var, written)) {
        return false;
    }

    return true;
}

static
void set_input_to_subj(
    ecs_rule_t *rule,
    ecs_rule_op_t *op,
    ecs_term_t *term,
    ecs_rule_var_t *var)
{
    (void)rule;
    
    op->has_in = true;
    if (!var) {
        op->r_in = UINT8_MAX;
        op->subject = term->subj.entity;

        /* Invalid entities should have been caught during parsing */
        ecs_assert(ecs_is_valid(rule->world, op->subject), 
            ECS_INTERNAL_ERROR, NULL);
    } else {
        op->r_in = var->id;
    }
}

static
void set_output_to_subj(
    ecs_rule_t *rule,
    ecs_rule_op_t *op,
    ecs_term_t *term,
    ecs_rule_var_t *var)
{
    (void)rule;

    op->has_out = true;
    if (!var) {
        op->r_out = UINT8_MAX;
        op->subject = term->subj.entity;

        /* Invalid entities should have been caught during parsing */
        ecs_assert(ecs_is_valid(rule->world, op->subject), 
            ECS_INTERNAL_ERROR, NULL);
    } else {
        op->r_out = var->id;
    }
}

static
void insert_select_or_with(
    ecs_rule_t *rule,
    int32_t c,
    ecs_term_t *term,
    ecs_rule_var_t *subj,
    ecs_rule_pair_t *pair,
    bool *written)
{
    ecs_rule_op_t *op;
    bool eval_subject_supersets = false;
    bool wildcard_subj = term->subj.entity == EcsWildcard;

    /* Find any entity and/or table variables for subject */
    ecs_rule_var_t *tvar = NULL, *evar = to_entity(rule, subj), *var = evar;
    if (subj && subj->kind == EcsRuleVarKindTable) {
        tvar = subj;
        if (!evar) {
            var = tvar;
        }
    }

    int32_t lbl_start = rule->operation_count;
    ecs_rule_pair_t filter;
    if (pair) {
        filter = *pair;
    } else {
        filter = term_to_pair(rule, term);
    }

    if (!var && !wildcard_subj) {
        /* Only insert implicit IsA if filter isn't already an IsA */
        if (!filter.transitive || filter.pred.ent != EcsIsA) {
            ecs_rule_pair_t isa_pair = {
                .pred.ent = EcsIsA,
                .obj.ent = term->subj.entity
            };
            evar = subj = store_inclusive_set(
                rule, EcsRuleSuperSet, &isa_pair, written, true);
            tvar = NULL;
            eval_subject_supersets = true;
        }
    }

    /* If no pair is provided, create operation from specified term */
    if (!pair) {
        op = insert_operation(rule, c, written);

    /* If an explicit pair is provided, override the default one from the 
     * term. This allows for using a predicate or object variable different
     * from what is in the term. One application of this is to substitute a
     * predicate with its subsets, if it is non final */
    } else {
        op = insert_operation(rule, -1, written);
        op->filter = *pair;

        /* Assign the term id, so that the operation will still be correctly
         * associated with the correct expression term. */
        op->term = c;
    }

    /* If entity variable is known and resolved, create with for it */
    if (evar && is_known(evar, written)) {
        op->kind = EcsRuleWith;
        op->r_in = evar->id;
        set_input_to_subj(rule, op, term, subj);

    /* If table variable is known and resolved, create with for it */
    } else if (tvar && is_known(tvar, written)) {
        op->kind = EcsRuleWith;
        op->r_in = tvar->id;
        set_input_to_subj(rule, op, term, subj);

    /* If subject is neither table nor entitiy, with operates on literal */        
    } else if (!tvar && !evar && !wildcard_subj) {
        op->kind = EcsRuleWith;
        set_input_to_subj(rule, op, term, subj);

    /* If subject is table or entity but not known, use select */
    } else {
        ecs_assert(wildcard_subj || subj != NULL, ECS_INTERNAL_ERROR, NULL);
        op->kind = EcsRuleSelect;
        if (!wildcard_subj) {
            set_output_to_subj(rule, op, term, subj);
            written[subj->id] = true;
        }
    }

    /* If supersets of subject are being evaluated, and we're looking for a
     * specific filter, stop as soon as the filter has been matched. */
    if (eval_subject_supersets && is_pair_known(rule, &op->filter, written)) {
        op = insert_operation(rule, -1, written);

        /* When the next operation returns, it will first hit SetJmp with a redo
         * which will switch the jump label to the previous operation */
        op->kind = EcsRuleSetJmp;
        op->on_pass = rule->operation_count;
        op->on_fail = lbl_start - 1;
    }

    if (op->filter.reg_mask & RULE_PAIR_PREDICATE) {
        written[op->filter.pred.reg] = true;
    }

    if (op->filter.reg_mask & RULE_PAIR_OBJECT) {
        written[op->filter.obj.reg] = true;
    }
}

static
void prepare_predicate(
    ecs_rule_t *rule,
    ecs_rule_pair_t *pair,
    bool *written)  
{
    /* If pair is not final, resolve term for all IsA relationships of the
     * predicate. Note that if the pair has final set to true, it is guaranteed
     * that the predicate can be used in an IsA query */
    if (!pair->final) {
        ecs_rule_pair_t isa_pair = {
            .pred.ent = EcsIsA,
            .obj.ent = pair->pred.ent
        };

        ecs_rule_var_t *pred = store_inclusive_set(
            rule, EcsRuleSubSet, &isa_pair, written, true);

        pair->pred.reg = pred->id;
        pair->reg_mask |= RULE_PAIR_PREDICATE;
    }
}

static
void insert_term_2(
    ecs_rule_t *rule,
    ecs_term_t *term,
    ecs_rule_pair_t *filter,
    int32_t c,
    bool *written)
{
    int32_t subj_id = -1, obj_id = -1;
    ecs_rule_var_t *subj = term_subj(rule, term);
    if ((subj = get_most_specific_var(rule, subj, written))) {
        subj_id = subj->id;
    }

    ecs_rule_var_t *obj = term_obj(rule, term);
    if ((obj = get_most_specific_var(rule, obj, written))) {
        obj_id = obj->id;
    }

    if (!filter->transitive) {
        insert_select_or_with(rule, c, term, subj, filter, written);

    } else if (filter->transitive) {
        if (is_known(subj, written)) {
            if (is_known(obj, written)) {
                ecs_rule_var_t *obj_subsets = store_inclusive_set(
                    rule, EcsRuleSubSet, filter, written, true);

                if (subj) {
                    subj = &rule->variables[subj_id];
                }

                ecs_rule_pair_t pair = *filter;
                pair.obj.reg = obj_subsets->id;
                pair.reg_mask |= RULE_PAIR_OBJECT;

                insert_select_or_with(rule, c, term, subj, &pair, written);
            } else {
                ecs_assert(obj != NULL, ECS_INTERNAL_ERROR, NULL);

                /* If subject is literal, find supersets for subject */
                if (subj == NULL || subj->kind == EcsRuleVarKindEntity) {
                    obj = to_entity(rule, obj);

                    ecs_rule_pair_t set_pair = *filter;
                    set_pair.reg_mask &= RULE_PAIR_PREDICATE;

                    if (subj) {
                        set_pair.obj.reg = subj->id;
                        set_pair.reg_mask |= RULE_PAIR_OBJECT;
                    } else {
                        set_pair.obj.ent = term->subj.entity;
                    }

                    insert_inclusive_set(rule, EcsRuleSuperSet, obj, set_pair, 
                        c, written, filter->inclusive);

                /* If subject is variable, first find matching pair for the 
                 * evaluated entity(s) and return supersets */
                } else {
                    ecs_rule_var_t *av = create_anonymous_variable(
                        rule, EcsRuleVarKindEntity);

                    subj = &rule->variables[subj_id];
                    obj = &rule->variables[obj_id];
                    obj = to_entity(rule, obj);

                    ecs_rule_pair_t set_pair = *filter;
                    set_pair.obj.reg = av->id;
                    set_pair.reg_mask |= RULE_PAIR_OBJECT;

                    /* Insert with to find initial object for relation */
                    insert_select_or_with(
                        rule, c, term, subj, &set_pair, written);

                    push_frame(rule);

                    /* Find supersets for returned initial object. Make sure
                     * this is always inclusive since it needs to return the
                     * object from the pair that the entity has itself. */
                    insert_inclusive_set(rule, EcsRuleSuperSet, obj, set_pair, 
                        c, written, true);
                }
            }

        /* subj is not known */
        } else {
            ecs_assert(subj != NULL, ECS_INTERNAL_ERROR, NULL);

            if (is_known(obj, written)) {
                ecs_rule_pair_t set_pair = *filter;
                set_pair.reg_mask &= RULE_PAIR_PREDICATE; /* clear object mask */

                if (obj) {
                    set_pair.obj.reg = obj->id;
                    set_pair.reg_mask |= RULE_PAIR_OBJECT;
                } else {
                    set_pair.obj.ent = term->obj.entity;
                }

                insert_inclusive_set(rule, EcsRuleSubSet, subj, set_pair, c, 
                    written, filter->inclusive);
            } else if (subj == obj) {
                insert_select_or_with(rule, c, term, subj, filter, written);
            } else {
                ecs_assert(obj != NULL, ECS_INTERNAL_ERROR, NULL);

                ecs_rule_var_t *av = create_anonymous_variable(
                    rule, EcsRuleVarKindEntity);

                subj = &rule->variables[subj_id];
                obj = &rule->variables[obj_id];
                obj = to_entity(rule, obj);

                /* TODO: this instruction currently does not return inclusive
                 * results. For example, it will return IsA(XWing, Machine) and
                 * IsA(XWing, Thing), but not IsA(XWing, XWing). To enable
                 * inclusive behavior, we need to be able to find all subjects
                 * that have IsA relationships, without expanding to all
                 * IsA relationships. For this a new mode needs to be supported
                 * where an operation never does a redo.
                 *
                 * This select can then be used to find all subjects, and those
                 * same subjects can then be used to find all (inclusive) 
                 * supersets for those subjects. */

                /* Insert instruction to find all subjects and objects */
                ecs_rule_op_t *op = insert_operation(rule, -1, written);
                op->kind = EcsRuleSelect;
                set_output_to_subj(rule, op, term, subj);

                /* Set object to anonymous variable */
                op->filter.pred = filter->pred;
                op->filter.obj.reg = av->id;
                op->filter.reg_mask = filter->reg_mask | RULE_PAIR_OBJECT;

                written[subj->id] = true;
                written[av->id] = true;

                /* Create new frame for operations that create inclusive set */
                push_frame(rule);

                /* Insert superset instruction to find all supersets */
                insert_inclusive_set(rule, EcsRuleSuperSet, obj, op->filter, c, 
                    written, true);
            }
        }
    }
}

static
void insert_term_1(
    ecs_rule_t *rule,
    ecs_term_t *term,
    ecs_rule_pair_t *filter,
    int32_t c,
    bool *written)
{
    ecs_rule_var_t *subj = term_subj(rule, term);
    subj = get_most_specific_var(rule, subj, written);
    insert_select_or_with(rule, c, term, subj, filter, written);
}

static
void insert_term(
    ecs_rule_t *rule,
    ecs_term_t *term,
    int32_t c,
    bool *written)
{
    bool obj_set = obj_is_set(term);

    ensure_most_specific_var(rule, term_pred(rule, term), written);
    if (obj_set) {
        ensure_most_specific_var(rule, term_obj(rule, term), written);
    }

    /* If term has Not operator, prepend Not which turns a fail into a pass */
    int32_t prev = rule->operation_count;
    ecs_rule_op_t *not_pre;
    if (term->oper == EcsNot) {
        not_pre = insert_operation(rule, -1, written);
        not_pre->kind = EcsRuleNot;
        not_pre->has_in = false;
        not_pre->has_out = false;
    }

    ecs_rule_pair_t filter = term_to_pair(rule, term);
    prepare_predicate(rule, &filter, written);

    if (subj_is_set(term) && !obj_set) {
        insert_term_1(rule, term, &filter, c, written);
    } else if (obj_set) {
        insert_term_2(rule, term, &filter, c, written);
    }

    /* If term has Not operator, append Not which turns a pass into a fail */
    if (term->oper == EcsNot) {
        ecs_rule_op_t *not_post = insert_operation(rule, -1, written);
        not_post->kind = EcsRuleNot;
        not_post->has_in = false;
        not_post->has_out = false;

        not_post->on_pass = prev - 1;
        not_post->on_fail = prev - 1;
        not_pre = &rule->operations[prev];
        not_pre->on_fail = rule->operation_count;
    }

    if (term->oper == EcsOptional) {
        /* Insert jump instruction that ensures that the optional term is only
         * executed once */
        ecs_rule_op_t *jump = insert_operation(rule, -1, written);
        jump->kind = EcsRuleNot;
        jump->has_in = false;
        jump->has_out = false;
        jump->on_pass = rule->operation_count;
        jump->on_fail = prev - 1;

        /* Find exit instruction for optional term, and make the fail label
         * point to the Not operation, so that even when the operation fails,
         * it won't discard the result */
        int i, min_fail = -1, exit_op = -1;
        for (i = prev; i < rule->operation_count; i ++) {
            ecs_rule_op_t *op = &rule->operations[i];
            if (min_fail == -1 || (op->on_fail >= 0 && op->on_fail < min_fail)){
                min_fail = op->on_fail;
                exit_op = i;
            }
        }

        ecs_assert(exit_op != -1, ECS_INTERNAL_ERROR, NULL);
        ecs_rule_op_t *op = &rule->operations[exit_op];
        op->on_fail = rule->operation_count - 1;
    }

    push_frame(rule);
}

/* Create program from operations that will execute the query */
static
void compile_program(
    ecs_rule_t *rule)
{
    /* Trace which variables have been written while inserting instructions.
     * This determines which instruction needs to be inserted */
    bool written[ECS_RULE_MAX_VARIABLE_COUNT] = { false };

    ecs_term_t *terms = rule->filter.terms;
    int32_t v, c, term_count = rule->filter.term_count;
    ecs_rule_op_t *op;

    /* Insert input, which is always the first instruction */
    insert_input(rule);

    /* First insert all instructions that do not have a variable subject. Such
     * instructions iterate the type of an entity literal and are usually good
     * candidates for quickly narrowing down the set of potential results. */
    for (c = 0; c < term_count; c ++) {
        ecs_term_t *term = &terms[c];
        if (skip_term(term)) {
            continue;
        }

        if (term->oper == EcsOptional) {
            continue;
        }

        ecs_rule_var_t* subj = term_subj(rule, term);
        if (subj) {
            continue;
        }

        if (term->subj.entity == EcsWildcard) {
            continue;
        }

        insert_term(rule, term, c, written);
    }

    /* Insert variables based on dependency order */
    for (v = 0; v < rule->subject_variable_count; v ++) {
        ecs_rule_var_t *var = &rule->variables[v];

        ecs_assert(var->kind == EcsRuleVarKindTable, ECS_INTERNAL_ERROR, NULL);

        for (c = 0; c < term_count; c ++) {
            ecs_term_t *term = &terms[c];
            if (skip_term(term)) {
                continue;
            }

            if (term->oper == EcsOptional) {
                continue;
            }

            /* Only process columns for which variable is subject */
            ecs_rule_var_t* subj = term_subj(rule, term);
            if (subj != var) {
                continue;
            }

            insert_term(rule, term, c, written);

            var = &rule->variables[v];
        }
    }

    /* Insert terms with wildcard subject */
    for (c = 0; c < term_count; c ++) {
        ecs_term_t *term = &terms[c];

        if (term->subj.entity != EcsWildcard) {
            continue;
        }

        insert_term(rule, term, c, written);
    }

    /* Insert terms with Not operators */
    for (c = 0; c < term_count; c ++) {
        ecs_term_t *term = &terms[c];
        if (term->oper != EcsNot) {
            continue;
        }

        insert_term(rule, term, c, written);
    }

    /* Insert terms with Optional operators last, as optional terms cannot 
     * eliminate results, and would just add overhead to evaluation of 
     * non-matching entities. */
    for (c = 0; c < term_count; c ++) {
        ecs_term_t *term = &terms[c];
        if (term->oper != EcsOptional) {
            continue;
        }
        
        insert_term(rule, term, c, written);
    }

    /* Verify all subject variables have been written. Subject variables are of
     * the table type, and a select/subset should have been inserted for each */
    for (v = 0; v < rule->subject_variable_count; v ++) {
        if (!written[v]) {
            /* If the table variable hasn't been written, this can only happen
             * if an instruction wrote the variable before a select/subset could
             * have been inserted for it. Make sure that this is the case by
             * testing if an entity variable exists and whether it has been
             * written. */
            ecs_rule_var_t *var = find_variable(
                rule, EcsRuleVarKindEntity, rule->variables[v].name);
            ecs_assert(written[var->id], ECS_INTERNAL_ERROR, var->name);
            (void)var;
        }
    }

    /* Make sure that all entity variables are written. With the exception of
     * the this variable, which can be returned as a table, other variables need
     * to be available as entities. This ensures that all permutations for all
     * variables are correctly returned by the iterator. When an entity variable
     * hasn't been written yet at this point, it is because it only constrained
     * through a common predicate or object. */
    for (; v < rule->variable_count; v ++) {
        if (!written[v]) {
            ecs_rule_var_t *var = &rule->variables[v];
            ecs_assert(var->kind == EcsRuleVarKindEntity, 
                ECS_INTERNAL_ERROR, NULL);

            ecs_rule_var_t *table_var = find_variable(
                rule, EcsRuleVarKindTable, var->name);
            
            /* A table variable must exist if the variable hasn't been resolved
             * yet. If there doesn't exist one, this could indicate an 
             * unconstrained variable which should have been caught earlier */
            ecs_assert(table_var != NULL, ECS_INTERNAL_ERROR, var->name);

            /* Insert each operation that takes the table variable as input, and
             * yields each entity in the table */
            op = insert_operation(rule, -1, written);
            op->kind = EcsRuleEach;
            op->r_in = table_var->id;
            op->r_out = var->id;
            op->frame = rule->frame_count;
            op->has_in = true;
            op->has_out = true;
            written[var->id] = true;
            
            push_frame(rule);
        }
    }     

    /* Insert yield, which is always the last operation */
    insert_yield(rule);
}

static
void create_variable_name_array(
    ecs_rule_t *rule)
{
    if (rule->variable_count) {
        rule->variable_names = ecs_os_malloc_n(char*, rule->variable_count);
        int i;
        for (i = 0; i < rule->variable_count; i ++) {
            ecs_rule_var_t *var = &rule->variables[i];

            if (var->kind != EcsRuleVarKindEntity) {
                /* Table variables are hidden for applications. */
                rule->variable_names[var->id] = NULL;
            } else {
                rule->variable_names[var->id] = var->name;
            }
        }
    }
}

/* Implementation for iterable mixin */
static
void rule_iter_init(
    const ecs_world_t *world,
    const ecs_poly_t *poly,
    ecs_iter_t *iter,
    ecs_term_t *filter)
{
    ecs_poly_assert(poly, ecs_rule_t);

    if (filter) {
        iter[1] = ecs_rule_iter(world, (ecs_rule_t*)poly);
        iter[0] = ecs_term_chain_iter(&iter[1], filter);
    } else {
        iter[0] = ecs_rule_iter(world, (ecs_rule_t*)poly);
    }
}

ecs_rule_t* ecs_rule_init(
    ecs_world_t *world,
    const ecs_filter_desc_t *desc)
{
    ecs_rule_t *result = ecs_poly_new(ecs_rule_t);

    /* Parse the signature expression. This initializes the columns array which
     * contains the information about which components/pairs are requested. */
    if (ecs_filter_init(world, &result->filter, desc)) {
        goto error;
    }

    result->world = world;

    /* Rule has no terms */
    if (!result->filter.term_count) {
        rule_error(result, "rule has no terms");
        goto error;
    }

    ecs_term_t *terms = result->filter.terms;
    int32_t i, term_count = result->filter.term_count;

    /* Make sure rule doesn't just have Not terms */
    for (i = 0; i < term_count; i++) {
        ecs_term_t *term = &terms[i];
        if (term->oper != EcsNot) {
            break;
        }
    }
    if (i == term_count) {
        rule_error(result, "rule cannot only have terms with Not operator");
        goto error;
    }

    /* Find all variables & resolve dependencies */
    if (scan_variables(result) != 0) {
        goto error;
    }

    /* Generate the opcode array */
    compile_program(result);

    /* Create array with variable names so this can be easily accessed by 
     * iterators without requiring access to the ecs_rule_t */
    create_variable_name_array(result);

    /* Create lookup array for subject variables */
    result->subject_variables = ecs_os_malloc_n(int32_t, term_count);

    for (i = 0; i < term_count; i ++) {
        ecs_term_t *term = &terms[i];
        if (term_id_is_variable(&term->subj)) {
            const char *subj_name = term_id_var_name(&term->subj);
            ecs_rule_var_t *subj = find_variable(
                result, EcsRuleVarKindEntity, subj_name);
            if (subj) {
                result->subject_variables[i] = subj->id;
                continue;
            }          
        }

        result->subject_variables[i] = -1;
    }

    result->iterable.init = rule_iter_init;

    return result;
error:
    ecs_rule_fini(result);
    return NULL;
}

void ecs_rule_fini(
    ecs_rule_t *rule)
{
    int32_t i;
    for (i = 0; i < rule->variable_count; i ++) {
        ecs_os_free(rule->variables[i].name);
    }

    ecs_os_free(rule->variables);
    ecs_os_free(rule->operations);
    ecs_os_free(rule->variable_names);
    ecs_os_free(rule->subject_variables);

    ecs_filter_fini(&rule->filter);

    ecs_os_free(rule);
}

const ecs_filter_t* ecs_rule_filter(
    const ecs_rule_t *rule)
{
    return &rule->filter; 
}

/* Quick convenience function to get a variable from an id */
static
ecs_rule_var_t* get_variable(
    const ecs_rule_t *rule,
    int32_t var_id)
{
    if (var_id == UINT8_MAX) {
        return NULL;
    }

    return &rule->variables[var_id];
}

/* Convert the program to a string. This can be useful to analyze how a rule is
 * being evaluated. */
char* ecs_rule_str(
    ecs_rule_t *rule)
{
    ecs_check(rule != NULL, ECS_INVALID_PARAMETER, NULL);
    
    ecs_world_t *world = rule->world;
    ecs_strbuf_t buf = ECS_STRBUF_INIT;
    char filter_expr[256];

    int32_t i, count = rule->operation_count;
    for (i = 1; i < count; i ++) {
        ecs_rule_op_t *op = &rule->operations[i];
        ecs_rule_pair_t pair = op->filter;
        ecs_entity_t pred = pair.pred.ent;
        ecs_entity_t obj = pair.obj.ent;
        const char *pred_name = NULL, *obj_name = NULL;
        char *pred_name_alloc = NULL, *obj_name_alloc = NULL;

        if (pair.reg_mask & RULE_PAIR_PREDICATE) {
            ecs_assert(rule->variables != NULL, ECS_INTERNAL_ERROR, NULL);
            ecs_rule_var_t *type_var = &rule->variables[pair.pred.reg];
            pred_name = type_var->name;
        } else if (pred) {
            pred_name_alloc = ecs_get_fullpath(world, ecs_get_alive(world, pred));
            pred_name = pred_name_alloc;
        }

        if (pair.reg_mask & RULE_PAIR_OBJECT) {
            ecs_assert(rule->variables != NULL, ECS_INTERNAL_ERROR, NULL);
            ecs_rule_var_t *obj_var = &rule->variables[pair.obj.reg];
            obj_name = obj_var->name;
        } else if (obj) {
            obj_name_alloc = ecs_get_fullpath(world, ecs_get_alive(world, obj));
            obj_name = obj_name_alloc;
        } else if (pair.obj_0) {
            obj_name = "0";
        }

        ecs_strbuf_append(&buf, "%2d: [S:%2d, P:%2d, F:%2d] ", i, 
            op->frame, op->on_pass, op->on_fail);

        bool has_filter = false;

        switch(op->kind) {
        case EcsRuleSelect:
            ecs_strbuf_append(&buf, "select   ");
            has_filter = true;
            break;
        case EcsRuleWith:
            ecs_strbuf_append(&buf, "with     ");
            has_filter = true;
            break;
        case EcsRuleStore:
            ecs_strbuf_append(&buf, "store    ");
            break;
        case EcsRuleSuperSet:
            ecs_strbuf_append(&buf, "superset ");
            has_filter = true;
            break;             
        case EcsRuleSubSet:
            ecs_strbuf_append(&buf, "subset   ");
            has_filter = true;
            break;            
        case EcsRuleEach:
            ecs_strbuf_append(&buf, "each     ");
            break;
        case EcsRuleSetJmp:
            ecs_strbuf_append(&buf, "setjmp   ");
            break;
        case EcsRuleJump:
            ecs_strbuf_append(&buf, "jump     ");
            break;
        case EcsRuleNot:
            ecs_strbuf_append(&buf, "not      ");
            break;            
        case EcsRuleYield:
            ecs_strbuf_append(&buf, "yield    ");
            break;
        default:
            continue;
        }

        if (op->has_out) {
            ecs_rule_var_t *r_out = get_variable(rule, op->r_out);
            if (r_out) {
                ecs_strbuf_append(&buf, "O:%s%s ", 
                    r_out->kind == EcsRuleVarKindTable ? "t" : "",
                    r_out->name);
            } else if (op->subject) {
                char *subj_path = ecs_get_fullpath(world, op->subject);
                ecs_strbuf_append(&buf, "O:%s ", subj_path);
                ecs_os_free(subj_path);
            }
        }

        if (op->has_in) {
            ecs_rule_var_t *r_in = get_variable(rule, op->r_in);
            if (r_in) {
                ecs_strbuf_append(&buf, "I:%s%s ", 
                    r_in->kind == EcsRuleVarKindTable ? "t" : "",
                    r_in->name);
            } else if (op->subject) {
                char *subj_path = ecs_get_fullpath(world, op->subject);
                ecs_strbuf_append(&buf, "I:%s ", subj_path);
                ecs_os_free(subj_path);
            }
        }

        if (has_filter) {
            if (!obj && !pair.obj_0) {
                ecs_os_sprintf(filter_expr, "(%s)", pred_name);
            } else {
                ecs_os_sprintf(filter_expr, "(%s, %s)", pred_name, obj_name);
            }
            ecs_strbuf_append(&buf, "F:%s", filter_expr);
        }

        ecs_strbuf_appendstr(&buf, "\n");

        ecs_os_free(pred_name_alloc);
        ecs_os_free(obj_name_alloc);
    }

    return ecs_strbuf_get(&buf);
error:
    return NULL;
}

/* Public function that returns number of variables. This enables an application
 * to iterate the variables and obtain their values. */
int32_t ecs_rule_var_count(
    const ecs_rule_t *rule)
{
    ecs_assert(rule != NULL, ECS_INTERNAL_ERROR, NULL);
    return rule->variable_count;
}

/* Public function to find a variable by name */
int32_t ecs_rule_find_var(
    const ecs_rule_t *rule,
    const char *name)
{
    ecs_rule_var_t *v = find_variable(rule, EcsRuleVarKindEntity, name);
    if (v) {
        return v->id;
    } else {
        return -1;
    }
}

/* Public function to get the name of a variable. */
const char* ecs_rule_var_name(
    const ecs_rule_t *rule,
    int32_t var_id)
{
    return rule->variables[var_id].name;
}

/* Public function to get the type of a variable. */
bool ecs_rule_var_is_entity(
    const ecs_rule_t *rule,
    int32_t var_id)
{
    return rule->variables[var_id].kind == EcsRuleVarKindEntity;
}

/* Public function to get the value of a variable. */
ecs_entity_t ecs_rule_get_var(
    ecs_iter_t *iter,
    int32_t var_id)
{
    ecs_rule_iter_t *it = &iter->priv.iter.rule;
    const ecs_rule_t *rule = it->rule;

    /* We can only return entity variables */
    if (rule->variables[var_id].kind == EcsRuleVarKindEntity) {
        ecs_rule_reg_t *regs = get_register_frame(it, rule->frame_count - 1);
        return entity_reg_get(rule, regs, var_id);
    } else {
        return 0;
    }
}

/* Public function to set the value of a variable before iterating. */
void ecs_rule_set_var(
    ecs_iter_t *it,
    int32_t var_id,
    ecs_entity_t value)
{
    ecs_check(it != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(var_id != -1, ECS_INVALID_PARAMETER, NULL);
    ecs_check(value != 0, ECS_INVALID_PARAMETER, NULL);
    /* Can't set variable while iterating */
    ecs_check(it->is_valid == false, ECS_INVALID_OPERATION, NULL);
    ecs_check(it->next == ecs_rule_next, ECS_INVALID_OPERATION, NULL);

    ecs_rule_iter_t *iter = &it->priv.iter.rule;
    const ecs_rule_t *r = iter->rule;
    ecs_check(var_id < r->variable_count, ECS_INVALID_PARAMETER, NULL);

    entity_reg_set(r, iter->registers, var_id, value);
error:
    return;
}

/* Create rule iterator */
ecs_iter_t ecs_rule_iter(
    const ecs_world_t *world,
    const ecs_rule_t *rule)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_assert(rule != NULL, ECS_INVALID_PARAMETER, NULL);

    ecs_iter_t result = {0};
    int i;

    result.world = (ecs_world_t*)world;
    result.real_world = (ecs_world_t*)ecs_get_world(rule->world);

    ecs_rule_iter_t *it = &result.priv.iter.rule;
    it->rule = rule;

    if (rule->operation_count) {
        if (rule->variable_count) {
            it->registers = ecs_os_malloc_n(ecs_rule_reg_t, 
                rule->operation_count * rule->variable_count);

            it->variables = ecs_os_malloc_n(ecs_entity_t, rule->variable_count);
        }
        
        it->op_ctx = ecs_os_calloc_n(ecs_rule_op_ctx_t, rule->operation_count);

        if (rule->filter.term_count) {
            it->columns = ecs_os_malloc_n(int32_t, 
                rule->operation_count * rule->filter.term_count);
        }

        for (i = 0; i < rule->filter.term_count; i ++) {
            it->columns[i] = -1;
        }
    }

    it->op = 0;

    for (i = 0; i < rule->variable_count; i ++) {
        if (rule->variables[i].kind == EcsRuleVarKindEntity) {
            entity_reg_set(rule, it->registers, i, EcsWildcard);
        } else {
            table_reg_set(rule, it->registers, i, NULL);
        }
    }

    result.variable_names = rule->variable_names;
    result.variable_count = rule->variable_count;
    result.term_count = rule->filter.term_count;
    result.terms = rule->filter.terms;
    result.next = ecs_rule_next;
    result.is_filter = rule->filter.filter;
    result.columns = it->columns; /* prevent alloc */

    return result;
}

void ecs_rule_iter_free(
    ecs_iter_t *iter)
{
    ecs_rule_iter_t *it = &iter->priv.iter.rule;
    ecs_os_free(it->registers);
    ecs_os_free(it->columns);
    ecs_os_free(it->op_ctx);
    ecs_os_free(it->variables);
    iter->columns = NULL;
    it->registers = NULL;
    it->columns = NULL;
    it->op_ctx = NULL;
    ecs_iter_fini(iter);
}

/* Edge case: if the filter has the same variable for both predicate and
 * object, they are both resolved at the same time but at the time of 
 * evaluating the filter they're still wildcards which would match columns
 * that have different predicates/objects. Do an additional scan to make
 * sure the column we're returning actually matches. */
static
int32_t find_next_same_var(
    ecs_type_t type,
    int32_t column,
    ecs_id_t pattern)
{
    /* If same_var is true, this has to be a wildcard pair. We cannot have
     * the same variable in a pair, and one part of a pair resolved with
     * another part unresolved. */
    ecs_assert(pattern == ecs_pair(EcsWildcard, EcsWildcard), 
        ECS_INTERNAL_ERROR, NULL);
    (void)pattern;
    
    /* Keep scanning for an id where rel and obj are the same */
    ecs_id_t *ids = ecs_vector_first(type, ecs_id_t);
    int32_t i, count = ecs_vector_count(type);
    for (i = column + 1; i < count; i ++) {
        ecs_id_t id = ids[i];
        if (!ECS_HAS_ROLE(id, PAIR)) {
            /* If id is not a pair, this will definitely not match, and we
             * will find no further matches. */
            return -1;
        }

        if (ECS_PAIR_RELATION(id) == ECS_PAIR_OBJECT(id)) {
            /* Found a match! */
            return i;
        }
    }

    /* No pairs found with same rel/obj */
    return -1;
}

static
int32_t find_next_column(
    const ecs_world_t *world,
    const ecs_table_t *table,
    int32_t column,
    ecs_rule_filter_t *filter)
{
    ecs_entity_t pattern = filter->mask;
    ecs_type_t type = table->type;

    if (column == -1) {
        ecs_table_record_t *tr = flecs_get_table_record(world, table, pattern);
        if (!tr) {
            return -1;
        }
        column = tr->column;
    } else {   
        column ++;

        if (ecs_vector_count(table->type) <= column) {
            return -1;
        }

        ecs_id_t *ids = ecs_vector_first(table->type, ecs_id_t);
        if (!ecs_id_match(ids[column], pattern)) {
            return -1;
        }
    }

    if (filter->same_var) {
        column = find_next_same_var(type, column, filter->mask);
    }

    return column;
}

/* This function finds the next table in a table set, and is used by the select
 * operation. The function automatically skips empty tables, so that subsequent
 * operations don't waste a lot of processing for nothing. */
static
ecs_table_record_t find_next_table(
    ecs_rule_filter_t *filter,
    ecs_rule_with_ctx_t *op_ctx)
{
    ecs_id_record_t *idr = op_ctx->idr;
    const ecs_table_record_t *tables = flecs_id_record_tables(idr);
    int32_t i = op_ctx->table_index, count = flecs_id_record_count(idr);
    ecs_table_t *table = NULL;
    int32_t column = -1;

    for (; i < count && (column == -1); i ++) {
        const ecs_table_record_t *tr = &tables[i];
        table = tr->table;

        /* Should only iterate non-empty tables */
        ecs_assert(ecs_table_count(table) != 0, ECS_INTERNAL_ERROR, NULL);

        column = tr->column;
        if (filter->same_var) {
            column = find_next_same_var(table->type, column - 1, filter->mask);
        }
    }

    if (column == -1) {
        table = NULL;
    }

    op_ctx->table_index = i;

    return (ecs_table_record_t){.table = table, .column = column};
}

static
ecs_id_record_t* find_tables(
    ecs_world_t *world,
    ecs_id_t id)
{
    ecs_id_record_t *idr = flecs_get_id_record(world, id);
    if (!flecs_id_record_count(idr)) {
        /* Skip ids that don't have (non-empty) tables */
        return NULL;
    }

    return idr;
}

static
ecs_id_t rule_get_column(
    ecs_type_t type,
    int32_t column)
{
    ecs_id_t *comp = ecs_vector_get(type, ecs_id_t, column);
    ecs_assert(comp != NULL, ECS_INTERNAL_ERROR, NULL);
    return *comp;
}

static
void set_column(
    ecs_iter_t *it,
    ecs_rule_op_t *op,
    ecs_type_t type,
    int32_t column)
{
    if (op->term == -1) {
        /* If operation is not associated with a term, don't set anything */
        return;
    }

    ecs_assert(op->term >= 0, ECS_INTERNAL_ERROR, NULL);

    if (type) {
        it->ids[op->term] = rule_get_column(type, column);
    } else {
        it->ids[op->term] = 0;
    }
}

static
void set_source(
    ecs_iter_t *it,
    ecs_rule_op_t *op,
    ecs_rule_reg_t *regs,
    int32_t r)
{
    if (op->term == -1) {
        /* If operation is not associated with a term, don't set anything */
        return;
    }

    ecs_assert(op->term >= 0, ECS_INTERNAL_ERROR, NULL);

    const ecs_rule_t *rule = it->priv.iter.rule.rule;
    if ((r != UINT8_MAX) && rule->variables[r].kind == EcsRuleVarKindEntity) {
        it->subjects[op->term] = reg_get_entity(rule, op, regs, r);
    } else {
        it->subjects[op->term] = 0;
    }
}

/* Input operation. The input operation acts as a placeholder for the start of
 * the program, and creates an entry in the register array that can serve to
 * store variables passed to an iterator. */
static
bool eval_input(
    ecs_iter_t *it,
    ecs_rule_op_t *op,
    int32_t op_index,
    bool redo)
{
    (void)it;
    (void)op;
    (void)op_index;

    if (!redo) {
        /* First operation executed by the iterator. Always return true. */
        return true;
    } else {
        /* When Input is asked to redo, it means that all other operations have
         * exhausted their results. Input itself does not yield anything, so
         * return false. This will terminate rule execution. */
        return false;
    }
}

static
bool eval_superset(
    ecs_iter_t *it,
    ecs_rule_op_t *op,
    int32_t op_index,
    bool redo)
{
    ecs_rule_iter_t *iter = &it->priv.iter.rule;
    const ecs_rule_t  *rule = iter->rule;
    ecs_world_t *world = rule->world;
    ecs_rule_superset_ctx_t *op_ctx = &iter->op_ctx[op_index].is.superset;
    ecs_rule_superset_frame_t *frame = NULL;
    ecs_rule_reg_t *regs = get_registers(iter, op);

    /* Get register indices for output */
    int32_t sp;
    int32_t r = op->r_out;

    /* Register cannot be a literal, since we need to store things in it */
    ecs_assert(r != UINT8_MAX, ECS_INTERNAL_ERROR, NULL);

    /* Superset results are always stored in an entity variable */
    ecs_assert(rule->variables[r].kind == EcsRuleVarKindEntity,    
        ECS_INTERNAL_ERROR, NULL);

    /* Get queried for id, fill out potential variables */
    ecs_rule_pair_t pair = op->filter;

    ecs_rule_filter_t filter = pair_to_filter(iter, op, pair);
    ecs_rule_filter_t super_filter = { 
        .mask = ecs_pair(ECS_PAIR_RELATION(filter.mask), EcsWildcard) 
    };
    ecs_table_t *table = NULL;

    if (!redo) {
        op_ctx->stack = op_ctx->storage;
        sp = op_ctx->sp = 0;
        frame = &op_ctx->stack[sp];

        /* Get table of object for which to get supersets */
        ecs_entity_t obj = ECS_PAIR_OBJECT(filter.mask);

        /* If obj is wildcard, there's nothing to determine a superset for */
        ecs_assert(obj != EcsWildcard, ECS_INTERNAL_ERROR, NULL);

        /* Find first matching column in table */
        table = table_from_entity(world, obj);
        int32_t column = find_next_column(world, table, -1, &super_filter);

        /* If no matching column was found, there are no supersets */
        if (column == -1) {
            return false;
        }

        ecs_entity_t col_entity = rule_get_column(table->type, column);
        ecs_entity_t col_obj = ecs_entity_t_lo(col_entity);

        entity_reg_set(rule, regs, r, col_obj);
        set_column(it, op, table->type, column);

        frame->table = table;
        frame->column = column;

        return true;
    }

    sp = op_ctx->sp;
    frame = &op_ctx->stack[sp];
    table = frame->table;
    int32_t column = frame->column;

    ecs_entity_t col_entity = rule_get_column(table->type, column);
    ecs_entity_t col_obj = ecs_entity_t_lo(col_entity);
    ecs_table_t *next_table = table_from_entity(world, col_obj);

    if (next_table) {
        sp ++;
        frame = &op_ctx->stack[sp];
        frame->table = next_table;
        frame->column = -1;
    }

    do {
        frame = &op_ctx->stack[sp];
        table = frame->table;
        column = frame->column;

        column = find_next_column(world, table, column, &super_filter);
        if (column != -1) {
            op_ctx->sp = sp;
            frame->column = column;
            col_entity = rule_get_column(table->type, column);
            col_obj = ecs_entity_t_lo(col_entity);

            entity_reg_set(rule, regs, r, col_obj);
            set_column(it, op, table->type, column);

            return true;        
        }

        sp --;
    } while (sp >= 0);

    return false;
}

static
bool eval_subset(
    ecs_iter_t *it,
    ecs_rule_op_t *op,
    int32_t op_index,
    bool redo)
{
    ecs_rule_iter_t *iter = &it->priv.iter.rule;
    const ecs_rule_t  *rule = iter->rule;
    ecs_world_t *world = rule->world;
    ecs_rule_subset_ctx_t *op_ctx = &iter->op_ctx[op_index].is.subset;
    ecs_rule_subset_frame_t *frame = NULL;
    ecs_table_record_t table_record;
    ecs_rule_reg_t *regs = get_registers(iter, op);

    /* Get register indices for output */
    int32_t sp, row;
    int32_t r = op->r_out;
    ecs_assert(r != UINT8_MAX, ECS_INTERNAL_ERROR, NULL);

    /* Get queried for id, fill out potential variables */
    ecs_rule_pair_t pair = op->filter;
    ecs_rule_filter_t filter = pair_to_filter(iter, op, pair);
    ecs_id_record_t *idr;
    ecs_table_t *table = NULL;

    if (!redo) {
        op_ctx->stack = op_ctx->storage;
        sp = op_ctx->sp = 0;
        frame = &op_ctx->stack[sp];
        idr = frame->with_ctx.idr = find_tables(world, filter.mask);
        if (!idr) {
            return false;
        }

        frame->with_ctx.table_index = 0;
        table_record = find_next_table(&filter, &frame->with_ctx);
        
        /* If first table set has no non-empty table, yield nothing */
        if (!table_record.table) {
            return false;
        }

        frame->row = 0;
        frame->column = table_record.column;
        table_reg_set(rule, regs, r, (frame->table = table_record.table));
        set_column(it, op, table_record.table->type, table_record.column);
        return true;
    }

    do {
        sp = op_ctx->sp;
        frame = &op_ctx->stack[sp];
        table = frame->table;
        row = frame->row;

        /* If row exceeds number of elements in table, find next table in frame that
         * still has entities */
        while ((sp >= 0) && (row >= ecs_table_count(table))) {
            table_record = find_next_table(&filter, &frame->with_ctx);

            if (table_record.table) {
                table = frame->table = table_record.table;
                ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);
                frame->row = 0;
                frame->column = table_record.column;
                set_column(it, op, table_record.table->type, table_record.column);
                table_reg_set(rule, regs, r, table);
                return true;
            } else {
                sp = -- op_ctx->sp;
                if (sp < 0) {
                    /* If none of the frames yielded anything, no more data */
                    return false;
                }
                frame = &op_ctx->stack[sp];
                table = frame->table;
                idr = frame->with_ctx.idr;
                row = ++ frame->row;

                ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);
                ecs_assert(idr != NULL, ECS_INTERNAL_ERROR, NULL);
            }
        }

        int32_t row_count = ecs_table_count(table);

        /* Table must have at least row elements */
        ecs_assert(row_count > row, ECS_INTERNAL_ERROR, NULL);

        ecs_entity_t *entities = ecs_vector_first(
            table->storage.entities, ecs_entity_t);
        ecs_assert(entities != NULL, ECS_INTERNAL_ERROR, NULL);

        /* The entity used to find the next table set */
        do {
            ecs_entity_t e = entities[row];

            /* Create look_for expression with the resolved entity as object */
            pair.reg_mask &= ~RULE_PAIR_OBJECT; /* turn of bit because it's not a reg */
            pair.obj.ent = e;
            filter = pair_to_filter(iter, op, pair);

            /* Find table set for expression */
            table = NULL;
            idr = find_tables(world, filter.mask);

            /* If table set is found, find first non-empty table */
            if (idr) {
                ecs_rule_subset_frame_t *new_frame = &op_ctx->stack[sp + 1];
                new_frame->with_ctx.idr = idr;
                new_frame->with_ctx.table_index = 0;
                table_record = find_next_table(&filter, &new_frame->with_ctx);

                /* If set contains non-empty table, push it to stack */
                if (table_record.table) {
                    table = table_record.table;
                    op_ctx->sp ++;
                    new_frame->table = table;
                    new_frame->row = 0;
                    new_frame->column = table_record.column;
                    frame = new_frame;
                }
            }

            /* If no table was found for the current entity, advance row */
            if (!table) {
                row = ++ frame->row;
            }
        } while (!table && row < row_count);
    } while (!table);

    table_reg_set(rule, regs, r, table);
    set_column(it, op, table->type, frame->column);

    return true;
}

/* Select operation. The select operation finds and iterates a table set that
 * corresponds to its pair expression.  */
static
bool eval_select(
    ecs_iter_t *it,
    ecs_rule_op_t *op,
    int32_t op_index,
    bool redo)
{
    ecs_rule_iter_t *iter = &it->priv.iter.rule;
    const ecs_rule_t  *rule = iter->rule;
    ecs_world_t *world = rule->world;
    ecs_rule_with_ctx_t *op_ctx = &iter->op_ctx[op_index].is.with;
    ecs_table_record_t table_record;
    ecs_rule_reg_t *regs = get_registers(iter, op);

    /* Get register indices for output */
    int32_t r = op->r_out;
    ecs_assert(r != UINT8_MAX, ECS_INTERNAL_ERROR, NULL);

    /* Get queried for id, fill out potential variables */
    ecs_rule_pair_t pair = op->filter;
    ecs_rule_filter_t filter = pair_to_filter(iter, op, pair);
    ecs_entity_t pattern = filter.mask;
    int32_t *columns = rule_get_columns(iter, op);

    int32_t column = -1;
    ecs_table_t *table = NULL;
    ecs_id_record_t *idr;

    if (!redo && op->term != -1) {
        it->ids[op->term] = pattern;
        columns[op->term] = -1;
    }

    /* If this is a redo, we already looked up the table set */
    if (redo) {
        idr = op_ctx->idr;
    
    /* If this is not a redo lookup the table set. Even though this may not be
     * the first time the operation is evaluated, variables may have changed
     * since last time, which could change the table set to lookup. */
    } else {
        /* A table set is a set of tables that all contain at least the 
         * requested look_for expression. What is returned is a table record, 
         * which in addition to the table also stores the first occurrance at
         * which the requested expression occurs in the table. This reduces (and
         * in most cases eliminates) any searching that needs to occur in a
         * table type. Tables are also registered under wildcards, which is why
         * this operation can simply use the look_for variable directly */

        idr = op_ctx->idr = find_tables(world, pattern);
    }

    /* If no table set was found for queried for entity, there are no results */
    if (!idr) {
        return false;
    }

    /* If this is not a redo, start at the beginning */
    if (!redo) {
        op_ctx->table_index = 0;

        /* Return the first table_record in the table set. */
        table_record = find_next_table(&filter, op_ctx);
    
        /* If no table record was found, there are no results. */
        if (!table_record.table) {
            return false;
        }

        table = table_record.table;

        /* Set current column to first occurrence of queried for entity */
        column = columns[op->term] = table_record.column;

        /* Store table in register */
        table_reg_set(rule, regs, r, table);
    
    /* If this is a redo, progress to the next match */
    } else {
        /* First test if there are any more matches for the current table, in 
         * case we're looking for a wildcard. */
        if (filter.wildcard) {
            table = table_reg_get(rule, regs, r);
            ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);

            column = columns[op->term];
            column = find_next_column(world, table, column, &filter);
            columns[op->term] = column;
        }

        /* If no next match was found for this table, move to next table */
        if (column == -1) {
            table_record = find_next_table(&filter, op_ctx);
            if (!table_record.table) {
                return false;
            }

            /* Assign new table to table register */
            table_reg_set(rule, regs, r, (table = table_record.table));

            /* Assign first matching column */
            column = columns[op->term] = table_record.column;
        }
    }

    /* If we got here, we found a match. Table and column must be set */
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(column != -1, ECS_INTERNAL_ERROR, NULL);

    /* If this is a wildcard query, fill out the variable registers */
    if (filter.wildcard) {
        reify_variables(iter, op, &filter, table->type, column);
    }
    
    if (!pair.obj_0) {
        set_column(it, op, table->type, column);
    }

    return true;
}

/* With operation. The With operation always comes after either the Select or
 * another With operation, and applies additional filters to the table. */
static
bool eval_with(
    ecs_iter_t *it,
    ecs_rule_op_t *op,
    int32_t op_index,
    bool redo)
{
    ecs_rule_iter_t *iter = &it->priv.iter.rule;
    const ecs_rule_t *rule = iter->rule;
    ecs_world_t *world = rule->world;
    ecs_rule_with_ctx_t *op_ctx = &iter->op_ctx[op_index].is.with;
    ecs_rule_reg_t *regs = get_registers(iter, op);

    /* Get register indices for input */
    int32_t r = op->r_in;

    /* Get queried for id, fill out potential variables */
    ecs_rule_pair_t pair = op->filter;
    ecs_rule_filter_t filter = pair_to_filter(iter, op, pair);
    int32_t *columns = rule_get_columns(iter, op);

    /* If looked for entity is not a wildcard (meaning there are no unknown/
     * unconstrained variables) and this is a redo, nothing more to yield. */
    if (redo && !filter.wildcard) {
        return false;
    }

    int32_t column = -1;
    ecs_table_t *table = NULL;
    ecs_id_record_t *idr;

    if (!redo && op->term != -1) {
        columns[op->term] = -1;
    }

    /* If this is a redo, we already looked up the table set */
    if (redo) {
        idr = op_ctx->idr;
    
    /* If this is not a redo lookup the table set. Even though this may not be
     * the first time the operation is evaluated, variables may have changed
     * since last time, which could change the table set to lookup. */
    } else {
        /* Transitive queries are inclusive, which means that if we have a
         * transitive predicate which is provided with the same subject and
         * object, it should return true. By default with will not return true
         * as the subject likely does not have itself as a relationship, which
         * is why this is a special case. 
         *
         * TODO: might want to move this code to a separate with_inclusive
         * instruction to limit branches for non-transitive queries (and to keep
         * code more readable).
         */
        if (pair.transitive && pair.inclusive) {
            ecs_entity_t subj = 0, obj = 0;
            
            if (r == UINT8_MAX) {
                subj = op->subject;
            } else {
                ecs_rule_var_t *v_subj = &rule->variables[r];
                if (v_subj->kind == EcsRuleVarKindEntity) {
                    subj = entity_reg_get(rule, regs, r);

                    /* This is the input for the op, so should always be set */
                    ecs_assert(subj != 0, ECS_INTERNAL_ERROR, NULL);
                }
            }

            /* If subj is set, it means that it is an entity. Try to also 
             * resolve the object. */
            if (subj) {
                /* If the object is not a wildcard, it has been reified. Get the
                 * value from either the register or as a literal */
                if (!filter.obj_wildcard) {
                    obj = ecs_entity_t_lo(filter.mask);
                    if (subj == obj) {
                        it->ids[op->term] = filter.mask;
                        return true;
                    }
                }
            }
        }

        /* The With operation finds the table set that belongs to its pair
         * filter. The table set is a sparse set that provides an O(1) operation
         * to check whether the current table has the required expression. */
        idr = op_ctx->idr = find_tables(world, filter.mask);
    }

    /* If no table set was found for queried for entity, there are no results. 
     * If this result is a transitive query, the table we're evaluating may not
     * be in the returned table set. Regardless, if the filter that contains a
     * transitive predicate does not have any tables associated with it, there
     * can be no transitive matches for the filter.  */
    if (!idr) {
        return false;
    }

    table = reg_get_table(rule, op, regs, r);
    if (!table) {
        return false;
    }

    /* If this is not a redo, start at the beginning */
    if (!redo) {
        column = find_next_column(world, table, -1, &filter);
    
    /* If this is a redo, progress to the next match */
    } else {        
        if (!filter.wildcard) {
            return false;
        }
        
        /* Find the next match for the expression in the column. The columns
         * array keeps track of the state for each With operation, so that
         * even after redoing a With, the search doesn't have to start from
         * the beginning. */
        column = find_next_column(world, table, columns[op->term], &filter);
    }

    /* If no next match was found for this table, no more data */
    if (column == -1) {
        return false;
    }

    columns[op->term] = column;

    /* If we got here, we found a match. Table and column must be set */
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(column != -1, ECS_INTERNAL_ERROR, NULL);

    /* If this is a wildcard query, fill out the variable registers */
    if (filter.wildcard) {
        reify_variables(iter, op, &filter, table->type, column);
    }

    if (!pair.obj_0) {
        set_column(it, op, table->type, column);
    }

    set_source(it, op, regs, r);

    return true;
}

/* Each operation. The each operation is a simple operation that takes a table
 * as input, and outputs each of the entities in a table. This operation is
 * useful for rules that match a table, and where the entities of the table are
 * used as predicate or object. If a rule contains an each operation, an
 * iterator is guaranteed to yield an entity instead of a table. The input for
 * an each operation can only be the root variable. */
static
bool eval_each(
    ecs_iter_t *it,
    ecs_rule_op_t *op,
    int32_t op_index,
    bool redo)
{
    ecs_rule_iter_t *iter = &it->priv.iter.rule;
    ecs_rule_each_ctx_t *op_ctx = &iter->op_ctx[op_index].is.each;
    ecs_rule_reg_t *regs = get_registers(iter, op);
    int32_t r_in = op->r_in;
    int32_t r_out = op->r_out;
    ecs_entity_t e;

    /* Make sure in/out registers are of the correct kind */
    ecs_assert(iter->rule->variables[r_in].kind == EcsRuleVarKindTable, 
        ECS_INTERNAL_ERROR, NULL);
    ecs_assert(iter->rule->variables[r_out].kind == EcsRuleVarKindEntity, 
        ECS_INTERNAL_ERROR, NULL);

    /* Get table, make sure that it contains data. The select operation should
     * ensure that empty tables are never forwarded. */
    ecs_table_t *table = table_reg_get(iter->rule, regs, r_in);
    if (table) {       
        int32_t row, count = regs[r_in].count;
        int32_t offset = regs[r_in].offset;

        if (!count) {
            count = ecs_table_count(table);
            ecs_assert(count != 0, ECS_INTERNAL_ERROR, NULL);
        } else {
            count += offset;
        }

        ecs_entity_t *entities = ecs_vector_first(
            table->storage.entities, ecs_entity_t);
        ecs_assert(entities != NULL, ECS_INTERNAL_ERROR, NULL);

        /* If this is is not a redo, start from row 0, otherwise go to the
        * next entity. */
        if (!redo) {
            row = op_ctx->row = offset;
        } else {
            row = ++ op_ctx->row;
        }

        /* If row exceeds number of entities in table, return false */
        if (row >= count) {
            return false;
        }

        /* Skip builtin entities that could confuse operations */
        e = entities[row];
        while (e == EcsWildcard || e == EcsThis) {
            row ++;
            if (row == count) {
                return false;
            }
            e = entities[row];      
        }
    } else {
        if (!redo) {
            e = entity_reg_get(iter->rule, regs, r_in);
        } else {
            return false;
        }
    }

    /* Assign entity */
    entity_reg_set(iter->rule, regs, r_out, e);

    return true;
}

/* Store operation. Stores entity in register. This can either be an entity 
 * literal or an entity variable that will be stored in a table register. The
 * latter facilitates scenarios where an iterator only need to return a single
 * entity but where the Yield returns tables. */
static
bool eval_store(
    ecs_iter_t *it,
    ecs_rule_op_t *op,
    int32_t op_index,
    bool redo)
{
    (void)op_index;

    if (redo) {
        /* Only ever return result once */
        return false;
    }

    ecs_rule_iter_t *iter = &it->priv.iter.rule;
    const ecs_rule_t *rule = iter->rule;
    ecs_rule_reg_t *regs = get_registers(iter, op);
    int32_t r_in = op->r_in;
    int32_t r_out = op->r_out;

    ecs_entity_t e = reg_get_entity(rule, op, regs, r_in);
    reg_set_entity(rule, regs, r_out, e);

    if (op->term >= 0) {
        ecs_rule_filter_t filter = pair_to_filter(iter, op, op->filter);
        it->ids[op->term] = filter.mask;
    }

    return true;
}

/* A setjmp operation sets the jump label for a subsequent jump label. When the
 * operation is first evaluated (redo=false) it sets the label to the on_pass
 * label, and returns true. When the operation is evaluated again (redo=true)
 * the label is set to on_fail and the operation returns false. */
static
bool eval_setjmp(
    ecs_iter_t *it,
    ecs_rule_op_t *op,
    int32_t op_index,
    bool redo)
{
    ecs_rule_iter_t *iter = &it->priv.iter.rule;
    ecs_rule_setjmp_ctx_t *ctx = &iter->op_ctx[op_index].is.setjmp;

    if (!redo) {
        ctx->label = op->on_pass;
        return true;
    } else {
        ctx->label = op->on_fail;
        return false;
    }
}

/* The jump operation jumps to an operation label. The operation always returns
 * true. Since the operation modifies the control flow of the program directly, 
 * the dispatcher does not look at the on_pass or on_fail labels of the jump
 * instruction. Instead, the on_pass label is used to store the label of the
 * operation that contains the label to jump to. */
static
bool eval_jump(
    ecs_iter_t *it,
    ecs_rule_op_t *op,
    int32_t op_index,
    bool redo)
{
    (void)it;
    (void)op;
    (void)op_index;

    /* Passthrough, result is not used for control flow */
    return !redo;
}

/* The not operation reverts the result of the operation it embeds */
static
bool eval_not(
    ecs_iter_t *it,
    ecs_rule_op_t *op,
    int32_t op_index,
    bool redo)
{
    (void)it;
    (void)op;
    (void)op_index;

    return !redo;
}

/* Yield operation. This is the simplest operation, as all it does is return
 * false. This will move the solver back to the previous instruction which
 * forces redo's on previous operations, for as long as there are matching
 * results. */
static
bool eval_yield(
    ecs_iter_t *it,
    ecs_rule_op_t *op,
    int32_t op_index,
    bool redo)
{
    (void)it;
    (void)op;
    (void)op_index;
    (void)redo;

    /* Yield always returns false, because there are never any operations after
     * a yield. */
    return false;
}

/* Dispatcher for operations */
static
bool eval_op(
    ecs_iter_t *it,
    ecs_rule_op_t *op,
    int32_t op_index,
    bool redo)
{
    switch(op->kind) {
    case EcsRuleInput:
        return eval_input(it, op, op_index, redo);
    case EcsRuleSelect:
        return eval_select(it, op, op_index, redo);
    case EcsRuleWith:
        return eval_with(it, op, op_index, redo);
    case EcsRuleSubSet:
        return eval_subset(it, op, op_index, redo);
    case EcsRuleSuperSet:
        return eval_superset(it, op, op_index, redo);
    case EcsRuleEach:
        return eval_each(it, op, op_index, redo);
    case EcsRuleStore:
        return eval_store(it, op, op_index, redo);
    case EcsRuleSetJmp:
        return eval_setjmp(it, op, op_index, redo);
    case EcsRuleJump:
        return eval_jump(it, op, op_index, redo);
    case EcsRuleNot:
        return eval_not(it, op, op_index, redo);
    case EcsRuleYield:
        return eval_yield(it, op, op_index, redo);
    default:
        return false;
    }
}

/* Utility to copy all registers to the next frame. Keeping track of register
 * values for each operation is necessary, because if an operation is asked to
 * redo matching, it must to be able to pick up from where it left of */
static
void push_registers(
    ecs_rule_iter_t *it,
    int32_t cur,
    int32_t next)
{
    if (!it->rule->variable_count) {
        return;
    }

    ecs_rule_reg_t *src_regs = get_register_frame(it, cur);
    ecs_rule_reg_t *dst_regs = get_register_frame(it, next);

    ecs_os_memcpy_n(dst_regs, src_regs, 
        ecs_rule_reg_t, it->rule->variable_count);
}

/* Utility to copy all columns to the next frame. Columns keep track of which
 * columns are currently being evaluated for a table, and are populated by the
 * Select and With operations. The columns array is important, as it is used
 * to tell the application where to find component data. */
static
void push_columns(
    ecs_rule_iter_t *it,
    int32_t cur,
    int32_t next)
{
    if (!it->rule->filter.term_count) {
        return;
    }

    int32_t *src_cols = rule_get_columns_frame(it, cur);
    int32_t *dst_cols = rule_get_columns_frame(it, next);

    ecs_os_memcpy_n(dst_cols, src_cols, int32_t, it->rule->filter.term_count);
}

/* Populate iterator with data before yielding to application */
static
void populate_iterator(
    const ecs_rule_t *rule,
    ecs_iter_t *iter,
    ecs_rule_iter_t *it,
    ecs_rule_op_t *op)
{
    ecs_world_t *world = rule->world;
    int32_t r = op->r_in;
    ecs_rule_reg_t *regs = get_register_frame(it, op->frame);
    ecs_table_t *table = NULL;
    int32_t count = 0;
    int32_t offset = 0;

    /* If the input register for the yield does not point to a variable,
     * the rule doesn't contain a this (.) variable. In that case, the
     * iterator doesn't contain any data, and this function will simply
     * return true or false. An application will still be able to obtain
     * the variables that were resolved. */
    if (r != UINT8_MAX) {
        ecs_rule_var_t *var = &rule->variables[r];
        ecs_rule_reg_t *reg = &regs[r];

        if (var->kind == EcsRuleVarKindTable) {
            table = table_reg_get(rule, regs, r);
            count = regs[r].count;
            offset = regs[r].offset;
        } else {
            /* If a single entity is returned, simply return the
             * iterator with count 1 and a pointer to the entity id */
            ecs_assert(var->kind == EcsRuleVarKindEntity, 
                ECS_INTERNAL_ERROR, NULL);

            ecs_entity_t e = reg->entity;
            ecs_record_t *record = ecs_eis_get(world, e);
            offset = ECS_RECORD_TO_ROW(record->row);

            /* If an entity is not stored in a table, it could not have
             * been matched by anything */
            ecs_assert(record != NULL, ECS_INTERNAL_ERROR, NULL);
            table = record->table;
            count = 1;
        }
    }

    int32_t i, variable_count = rule->variable_count;
    int32_t term_count = rule->filter.term_count;
    iter->variables = it->variables;

    for (i = 0; i < variable_count; i ++) {
        if (rule->variables[i].kind == EcsRuleVarKindEntity) {
            it->variables[i] = regs[i].entity;
        } else {
            it->variables[i] = 0;
        }
    }

    for (i = 0; i < term_count; i ++) {
        int32_t v = rule->subject_variables[i];
        if (v != -1) {
            ecs_rule_var_t *var = &rule->variables[v];
            if (var->kind == EcsRuleVarKindEntity) {
                iter->subjects[i] = regs[var->id].entity;
            }
        }
    }

    /* Iterator expects column indices to start at 1 */
    iter->columns = rule_get_columns_frame(it, op->frame);
    for (i = 0; i < term_count; i ++) {
        ecs_entity_t subj = iter->subjects[i];
        int32_t c = ++ iter->columns[i];

        if (!subj) {
            if (iter->terms[i].subj.entity != EcsThis) {
                iter->columns[i] = 0;
            }
        } else if (c) {
            iter->columns[i] = -1;
        }
    }

    flecs_iter_populate_data(world, iter, table, offset, count, 
        iter->ptrs, iter->sizes);
}

static
bool is_control_flow(
    ecs_rule_op_t *op)
{
    switch(op->kind) {
    case EcsRuleSetJmp:
    case EcsRuleJump:
        return true;
    default:
        return false;
    }
}

/* Iterator next function. This evaluates the program until it reaches a Yield
 * operation, and returns the intermediate result(s) to the application. An
 * iterator can, depending on the program, either return a table, entity, or
 * just true/false, in case a rule doesn't contain the this variable. */
bool ecs_rule_next(
    ecs_iter_t *it)
{
    ecs_check(it != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(it->next == ecs_rule_next, ECS_INVALID_PARAMETER, NULL);

    ecs_rule_iter_t *iter = &it->priv.iter.rule;
    const ecs_rule_t *rule = iter->rule;
    bool redo = iter->redo;
    int32_t last_frame = -1;
    bool init_subjects = it->subjects == NULL;

    /* Can't iterate an iterator that's already depleted */
    ecs_check(iter->op != -1, ECS_INVALID_PARAMETER, NULL);

    flecs_iter_init(it);

    /* Make sure that if there are any terms with literal subjects, they're
     * initialized in the subjects array */
    if (init_subjects) {
        int32_t i;
        for (i = 0; i < rule->filter.term_count; i ++) {
            ecs_term_t *t = &rule->filter.terms[i];
            ecs_term_id_t *subj = &t->subj;
            if (subj->var == EcsVarIsEntity && subj->entity != EcsThis) {
                it->subjects[i] = subj->entity;
            }
        }

        for (i = 0; i < rule->filter.term_count; i ++) {
            ecs_term_t *term = &rule->filter.terms[i];
            if (term->subj.set.mask & EcsNothing || 
                term->oper == EcsNot ||
                term->oper == EcsOptional ||
                term->id == ecs_pair(EcsChildOf, 0)) {
                it->ids[i] = term->id;
            }
        }
    }

    do {
        /* Evaluate an operation. The result of an operation determines the
         * flow of the program. If an operation returns true, the program 
         * continues to the operation pointed to by 'on_pass'. If the operation
         * returns false, the program continues to the operation pointed to by
         * 'on_fail'.
         *
         * In most scenarios, on_pass points to the next operation, and on_fail
         * points to the previous operation.
         *
         * When an operation fails, the previous operation will be invoked with
         * redo=true. This will cause the operation to continue its search from
         * where it left off. When the operation succeeds, the next operation
         * will be invoked with redo=false. This causes the operation to start
         * from the beginning, which is necessary since it just received a new
         * input. */
        int32_t op_index = iter->op;
        ecs_rule_op_t *op = &rule->operations[op_index];
        int32_t cur = op->frame;

        /* If this is not the first operation and is also not a control flow
         * operation, push a new frame on the stack for the next operation */
        if (!redo && !is_control_flow(op) && cur && cur != last_frame) {
            int32_t prev = cur - 1;
            push_registers(iter, prev, cur);
            push_columns(iter, prev, cur);
        }

        /* Dispatch the operation */
        bool result = eval_op(it, op, op_index, redo);
        iter->op = result ? op->on_pass : op->on_fail;

        /* If the current operation is yield, return results */
        if (op->kind == EcsRuleYield) {
            populate_iterator(rule, it, iter, op);
            iter->redo = true;
            return true;
        }

        /* If the current operation is a jump, goto stored label */
        if (op->kind == EcsRuleJump) {
            /* Label is stored in setjmp context */
            iter->op = iter->op_ctx[op->on_pass].is.setjmp.label;
        }

        /* If jumping backwards, it's a redo */
        redo = iter->op <= op_index;

        if (!is_control_flow(op)) {
            last_frame = op->frame;
        }
    } while (iter->op != -1);

    ecs_rule_iter_free(it);

error:
    return false;
}

#endif
