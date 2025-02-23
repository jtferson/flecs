#include "meta.h"

#ifdef FLECS_META

/* EcsMetaTypeSerialized lifecycle */

void ecs_meta_dtor_serialized(
    EcsMetaTypeSerialized *ptr) 
{
    int32_t i, count = ecs_vector_count(ptr->ops);
    ecs_meta_type_op_t *ops = ecs_vector_first(ptr->ops, ecs_meta_type_op_t);
    
    for (i = 0; i < count; i ++) {
        ecs_meta_type_op_t *op = &ops[i];
        if (op->members) {
            flecs_hashmap_free(*op->members);
            ecs_os_free(op->members);
        }
    }

    ecs_vector_free(ptr->ops); 
}

static ECS_COPY(EcsMetaTypeSerialized, dst, src, {
    ecs_meta_dtor_serialized(dst);

    dst->ops = ecs_vector_copy(src->ops, ecs_meta_type_op_t);

    int32_t o, count = ecs_vector_count(src->ops);
    ecs_meta_type_op_t *ops = ecs_vector_first(src->ops, ecs_meta_type_op_t);
    
    for (o = 0; o < count; o ++) {
        ecs_meta_type_op_t *op = &ops[o];
        if (op->members) {
            op->members = ecs_os_memdup_t(op->members, ecs_hashmap_t);
            *op->members = flecs_hashmap_copy(*op->members);
        }
    }
})

static ECS_MOVE(EcsMetaTypeSerialized, dst, src, {
    ecs_meta_dtor_serialized(dst);
    dst->ops = src->ops;
    src->ops = NULL;
})

static ECS_DTOR(EcsMetaTypeSerialized, ptr, { 
    ecs_meta_dtor_serialized(ptr);
})


/* EcsStruct lifecycle */

static void dtor_struct(
    EcsStruct *ptr) 
{
    ecs_member_t *members = ecs_vector_first(ptr->members, ecs_member_t);
    int32_t i, count = ecs_vector_count(ptr->members);
    for (i = 0; i < count; i ++) {
        ecs_os_free((char*)members[i].name);
    }
    ecs_vector_free(ptr->members);
}

static ECS_COPY(EcsStruct, dst, src, {
    dtor_struct(dst);

    dst->members = ecs_vector_copy(src->members, ecs_member_t);

    ecs_member_t *members = ecs_vector_first(dst->members, ecs_member_t);
    int32_t m, count = ecs_vector_count(dst->members);

    for (m = 0; m < count; m ++) {
        members[m].name = ecs_os_strdup(members[m].name);
    }
})

static ECS_MOVE(EcsStruct, dst, src, {
    dtor_struct(dst);
    dst->members = src->members;
    src->members = NULL;
})

static ECS_DTOR(EcsStruct, ptr, { dtor_struct(ptr); })


/* EcsEnum lifecycle */

static void dtor_enum(
    EcsEnum *ptr) 
{
    ecs_map_iter_t it = ecs_map_iter(ptr->constants);
    ecs_enum_constant_t *c;
    while ((c = ecs_map_next(&it, ecs_enum_constant_t, NULL))) {
        ecs_os_free((char*)c->name);
    }
    ecs_map_free(ptr->constants);
}

static ECS_COPY(EcsEnum, dst, src, {
    dtor_enum(dst);

    dst->constants = ecs_map_copy(src->constants);
    ecs_assert(ecs_map_count(dst->constants) == ecs_map_count(src->constants),
        ECS_INTERNAL_ERROR, NULL);

    ecs_map_iter_t it = ecs_map_iter(dst->constants);
    ecs_enum_constant_t *c;
    while ((c = ecs_map_next(&it, ecs_enum_constant_t, NULL))) {
        c->name = ecs_os_strdup(c->name);
    }
})

static ECS_MOVE(EcsEnum, dst, src, {
    dtor_enum(dst);
    dst->constants = src->constants;
    src->constants = NULL;
})

static ECS_DTOR(EcsEnum, ptr, { dtor_enum(ptr); })


/* EcsBitmask lifecycle */

static void dtor_bitmask(
    EcsBitmask *ptr) 
{
    ecs_map_iter_t it = ecs_map_iter(ptr->constants);
    ecs_bitmask_constant_t *c;
    while ((c = ecs_map_next(&it, ecs_bitmask_constant_t, NULL))) {
        ecs_os_free((char*)c->name);
    }
    ecs_map_free(ptr->constants);
}

static ECS_COPY(EcsBitmask, dst, src, {
    dtor_bitmask(dst);

    dst->constants = ecs_map_copy(src->constants);
    ecs_assert(ecs_map_count(dst->constants) == ecs_map_count(src->constants),
        ECS_INTERNAL_ERROR, NULL);

    ecs_map_iter_t it = ecs_map_iter(dst->constants);
    ecs_bitmask_constant_t *c;
    while ((c = ecs_map_next(&it, ecs_bitmask_constant_t, NULL))) {
        c->name = ecs_os_strdup(c->name);
    }
})

static ECS_MOVE(EcsBitmask, dst, src, {
    dtor_bitmask(dst);
    dst->constants = src->constants;
    src->constants = NULL;
})

static ECS_DTOR(EcsBitmask, ptr, { dtor_bitmask(ptr); })


/* Type initialization */

static
int init_type(
    ecs_world_t *world,
    ecs_entity_t type,
    ecs_type_kind_t kind)
{
    ecs_assert(world != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(type != 0, ECS_INTERNAL_ERROR, NULL);

    EcsMetaType *meta_type = ecs_get_mut(world, type, EcsMetaType, NULL);
    if (meta_type->kind && meta_type->kind != kind) {
        ecs_err("type '%s' reregistered with different kind", 
            ecs_get_name(world, type));
        return -1;
    }

    meta_type->kind = kind;
    ecs_modified(world, type, EcsMetaType);

    return 0;
}

static
int init_component(
    ecs_world_t *world,
    ecs_entity_t type,
    ecs_size_t size,
    ecs_size_t alignment)
{
    ecs_assert(world != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(type != 0, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(size != 0, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(alignment != 0, ECS_INTERNAL_ERROR, NULL);

    EcsComponent *component = ecs_get_mut(world, type, EcsComponent, NULL);
    if (component->size && component->size != size) {
        ecs_err("type '%s' reregistered with different size",
            ecs_get_name(world, type));
        return -1;
    }

    if (component->alignment && component->alignment != alignment) {
        ecs_err("type '%s' reregistered with different alignment",
            ecs_get_name(world, type));
        return -1;
    }

    component->size = size;
    component->alignment = alignment;
    ecs_modified(world, type, EcsComponent);

    return 0;
}

static
void set_struct_member(
    ecs_member_t *member,
    ecs_entity_t entity,
    const char *name,
    ecs_entity_t type,
    int32_t count)
{
    member->member = entity;
    member->type = type;
    member->count = count;

    if (!count) {
        member->count = 1;
    }

    ecs_os_strset((char**)&member->name, name);
}

static
int add_member_to_struct(
    ecs_world_t *world,
    ecs_entity_t type,
    ecs_entity_t member,
    EcsMember *m)
{
    ecs_assert(world != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(type != 0, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(member != 0, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(m != NULL, ECS_INTERNAL_ERROR, NULL);

    const char *name = ecs_get_name(world, member);
    if (!name) {
        char *path = ecs_get_fullpath(world, type);
        ecs_err("member for struct '%s' does not have a name", path);
        ecs_os_free(path);
        return -1;
    }

    if (!m->type) {
        char *path = ecs_get_fullpath(world, member);
        ecs_err("member '%s' does not have a type", path);
        ecs_os_free(path);
        return -1;
    }

    if (ecs_get_typeid(world, m->type) == 0) {
        char *path = ecs_get_fullpath(world, member);
        char *ent_path = ecs_get_fullpath(world, m->type);
        ecs_err("member '%s.type' is '%s' which is not a type", path, ent_path);
        ecs_os_free(path);
        ecs_os_free(ent_path);
        return -1;
    }

    EcsStruct *s = ecs_get_mut(world, type, EcsStruct, NULL);
    ecs_assert(s != NULL, ECS_INTERNAL_ERROR, NULL);

    /* First check if member is already added to struct */
    ecs_member_t *members = ecs_vector_first(s->members, ecs_member_t);
    int32_t i, count = ecs_vector_count(s->members);
    for (i = 0; i < count; i ++) {
        if (members[i].member == member) {
            set_struct_member(&members[i], member, name, m->type, m->count);
            break;
        }
    }

    /* If member wasn't added yet, add a new element to vector */
    if (i == count) {
        ecs_member_t *elem = ecs_vector_add(&s->members, ecs_member_t);
        elem->name = NULL;
        set_struct_member(elem, member, name, m->type, m->count);

        /* Reobtain members array in case it was reallocated */
        members = ecs_vector_first(s->members, ecs_member_t);
        count ++;
    }

    /* Compute member offsets and size & alignment of struct */
    ecs_size_t size = 0;
    ecs_size_t alignment = 0;

    for (i = 0; i < count; i ++) {
        ecs_member_t *elem = &members[i];

        ecs_assert(elem->name != NULL, ECS_INTERNAL_ERROR, NULL);
        ecs_assert(elem->type != 0, ECS_INTERNAL_ERROR, NULL);

        /* Get component of member type to get its size & alignment */
        const EcsComponent *mbr_comp = ecs_get(world, elem->type, EcsComponent);
        if (!mbr_comp) {
            char *path = ecs_get_fullpath(world, member);
            ecs_err("member '%s' is not a type", path);
            ecs_os_free(path);
            return -1;
        }

        ecs_size_t member_size = mbr_comp->size;
        ecs_size_t member_alignment = mbr_comp->alignment;

        if (!member_size || !member_alignment) {
            char *path = ecs_get_fullpath(world, member);
            ecs_err("member '%s' has 0 size/alignment");
            ecs_os_free(path);
            return -1;
        }

        member_size *= elem->count;
        size = ECS_ALIGN(size, member_alignment);
        elem->size = member_size;
        elem->offset = size;

        size += member_size;

        if (member_alignment > alignment) {
            alignment = member_alignment;
        }
    }

    if (size == 0) {
        ecs_err("struct '%s' has 0 size", ecs_get_name(world, type));
        return -1;
    }

    if (alignment == 0) {
        ecs_err("struct '%s' has 0 alignment", ecs_get_name(world, type));
        return -1;
    }

    /* Align struct size to struct alignment */
    size = ECS_ALIGN(size, alignment);

    ecs_modified(world, type, EcsStruct);

    /* Overwrite component size & alignment */
    if (type != ecs_id(EcsComponent)) {
        EcsComponent *comp = ecs_get_mut(world, type, EcsComponent, NULL);
        comp->size = size;
        comp->alignment = alignment;
        ecs_modified(world, type, EcsComponent);
    }

    /* Do this last as it triggers the update of EcsMetaTypeSerialized */
    if (init_type(world, type, EcsStructType)) {
        return -1;
    }

    /* If current struct is also a member, assign to itself */
    if (ecs_has(world, type, EcsMember)) {
        EcsMember *type_mbr = ecs_get_mut(world, type, EcsMember, NULL);
        ecs_assert(type_mbr != NULL, ECS_INTERNAL_ERROR, NULL);

        type_mbr->type = type;
        type_mbr->count = 1;

        ecs_modified(world, type, EcsMember);
    }

    return 0;
}

static
int add_constant_to_enum(
    ecs_world_t *world, 
    ecs_entity_t type, 
    ecs_entity_t e,
    ecs_id_t constant_id)
{
    EcsEnum *ptr = ecs_get_mut(world, type, EcsEnum, NULL);
    
    /* Remove constant from map if it was already added */
    ecs_map_iter_t it = ecs_map_iter(ptr->constants);
    ecs_enum_constant_t *c;
    ecs_map_key_t key;
    while ((c = ecs_map_next(&it, ecs_enum_constant_t, &key))) {
        if (c->constant == e) {
            ecs_os_free((char*)c->name);
            ecs_map_remove(ptr->constants, key);
        }
    }

    /* Check if constant sets explicit value */
    int32_t value = 0;
    bool value_set = false;
    if (ecs_id_is_pair(constant_id)) {
        if (ecs_pair_object(world, constant_id) != ecs_id(ecs_i32_t)) {
            char *path = ecs_get_fullpath(world, e);
            ecs_err("expected i32 type for enum constant '%s'", path);
            ecs_os_free(path);
            return -1;
        }

        const int32_t *value_ptr = ecs_get_pair_object(
            world, e, EcsConstant, ecs_i32_t);
        ecs_assert(value_ptr != NULL, ECS_INTERNAL_ERROR, NULL);
        value = *value_ptr;
        value_set = true;
    }

    /* Make sure constant value doesn't conflict if set / find the next value */
    it = ecs_map_iter(ptr->constants);
    while  ((c = ecs_map_next(&it, ecs_enum_constant_t, &key))) {
        if (value_set) {
            if (c->value == value) {
                char *path = ecs_get_fullpath(world, e);
                ecs_err("conflicting constant value for '%s' (other is '%s')",
                    path, c->name);
                ecs_os_free(path);
                return -1;
            }
        } else {
            if (c->value >= value) {
                value = c->value + 1;
            }
        }
    }

    if (!ptr->constants) {
        ptr->constants = ecs_map_new(ecs_enum_constant_t, 1);
    }

    c = ecs_map_ensure(ptr->constants, ecs_enum_constant_t, value);
    c->name = ecs_os_strdup(ecs_get_name(world, e));
    c->value = value;
    c->constant = e;

    ecs_i32_t *cptr = ecs_get_mut_pair_object(
        world, e, EcsConstant, ecs_i32_t, NULL);
    ecs_assert(cptr != NULL, ECS_INTERNAL_ERROR, NULL);
    cptr[0] = value;

    return 0;
}

static
int add_constant_to_bitmask(
    ecs_world_t *world, 
    ecs_entity_t type, 
    ecs_entity_t e,
    ecs_id_t constant_id)
{
    EcsBitmask *ptr = ecs_get_mut(world, type, EcsBitmask, NULL);
    
    /* Remove constant from map if it was already added */
    ecs_map_iter_t it = ecs_map_iter(ptr->constants);
    ecs_bitmask_constant_t *c;
    ecs_map_key_t key;
    while ((c = ecs_map_next(&it, ecs_bitmask_constant_t, &key))) {
        if (c->constant == e) {
            ecs_os_free((char*)c->name);
            ecs_map_remove(ptr->constants, key);
        }
    }

    /* Check if constant sets explicit value */
    uint32_t value = 1;
    if (ecs_id_is_pair(constant_id)) {
        if (ecs_pair_object(world, constant_id) != ecs_id(ecs_u32_t)) {
            char *path = ecs_get_fullpath(world, e);
            ecs_err("expected u32 type for bitmask constant '%s'", path);
            ecs_os_free(path);
            return -1;
        }

        const uint32_t *value_ptr = ecs_get_pair_object(
            world, e, EcsConstant, ecs_u32_t);
        ecs_assert(value_ptr != NULL, ECS_INTERNAL_ERROR, NULL);
        value = *value_ptr;
    } else {
        value = 1u << (ecs_u32_t)ecs_map_count(ptr->constants);
    }

    /* Make sure constant value doesn't conflict */
    it = ecs_map_iter(ptr->constants);
    while  ((c = ecs_map_next(&it, ecs_bitmask_constant_t, &key))) {
        if (c->value == value) {
            char *path = ecs_get_fullpath(world, e);
            ecs_err("conflicting constant value for '%s' (other is '%s')",
                path, c->name);
            ecs_os_free(path);
            return -1;
        }
    }

    if (!ptr->constants) {
        ptr->constants = ecs_map_new(ecs_bitmask_constant_t, 1);
    }

    c = ecs_map_ensure(ptr->constants, ecs_bitmask_constant_t, value);
    c->name = ecs_os_strdup(ecs_get_name(world, e));
    c->value = value;
    c->constant = e;

    ecs_u32_t *cptr = ecs_get_mut_pair_object(
        world, e, EcsConstant, ecs_u32_t, NULL);
    ecs_assert(cptr != NULL, ECS_INTERNAL_ERROR, NULL);
    cptr[0] = value;

    return 0;
}

static
void set_primitive(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    EcsPrimitive *type = ecs_term(it, EcsPrimitive, 1);

    int i, count = it->count;
    for (i = 0; i < count; i ++) {
        ecs_entity_t e = it->entities[i];
        switch(type->kind) {
        case EcsBool:
            init_component(world, e, 
                ECS_SIZEOF(bool), ECS_ALIGNOF(bool));
            init_type(world, e, EcsPrimitiveType);
            break;
        case EcsChar:
            init_component(world, e, 
                ECS_SIZEOF(char), ECS_ALIGNOF(char));
            init_type(world, e, EcsPrimitiveType);
            break;
        case EcsByte:
            init_component(world, e, 
                ECS_SIZEOF(bool), ECS_ALIGNOF(bool));
            init_type(world, e, EcsPrimitiveType);
            break;
        case EcsU8:
            init_component(world, e, 
                ECS_SIZEOF(uint8_t), ECS_ALIGNOF(uint8_t));
            init_type(world, e, EcsPrimitiveType);
            break;
        case EcsU16:
            init_component(world, e, 
                ECS_SIZEOF(uint16_t), ECS_ALIGNOF(uint16_t));
            init_type(world, e, EcsPrimitiveType);
            break;
        case EcsU32:
            init_component(world, e, 
                ECS_SIZEOF(uint32_t), ECS_ALIGNOF(uint32_t));
            init_type(world, e, EcsPrimitiveType);
            break;
        case EcsU64:
            init_component(world, e, 
                ECS_SIZEOF(uint64_t), ECS_ALIGNOF(uint64_t));
            init_type(world, e, EcsPrimitiveType);
            break;
        case EcsI8:
            init_component(world, e, 
                ECS_SIZEOF(int8_t), ECS_ALIGNOF(int8_t));
            init_type(world, e, EcsPrimitiveType);
            break;
        case EcsI16:
            init_component(world, e, 
                ECS_SIZEOF(int16_t), ECS_ALIGNOF(int16_t));
            init_type(world, e, EcsPrimitiveType);
            break;
        case EcsI32:
            init_component(world, e, 
                ECS_SIZEOF(int32_t), ECS_ALIGNOF(int32_t));
            init_type(world, e, EcsPrimitiveType);
            break;
        case EcsI64:
            init_component(world, e, 
                ECS_SIZEOF(int64_t), ECS_ALIGNOF(int64_t));
            init_type(world, e, EcsPrimitiveType);
            break;
        case EcsF32:
            init_component(world, e, 
                ECS_SIZEOF(float), ECS_ALIGNOF(float));
            init_type(world, e, EcsPrimitiveType);
            break;
        case EcsF64:
            init_component(world, e, 
                ECS_SIZEOF(double), ECS_ALIGNOF(double));
            init_type(world, e, EcsPrimitiveType);
            break;
        case EcsUPtr:
            init_component(world, e, 
                ECS_SIZEOF(uintptr_t), ECS_ALIGNOF(uintptr_t));
            init_type(world, e, EcsPrimitiveType);
            break;
        case EcsIPtr:
            init_component(world, e, 
                ECS_SIZEOF(intptr_t), ECS_ALIGNOF(intptr_t));
            init_type(world, e, EcsPrimitiveType);
            break;
        case EcsString:
            init_component(world, e, 
                ECS_SIZEOF(char*), ECS_ALIGNOF(char*));
            init_type(world, e, EcsPrimitiveType);
            break;
        case EcsEntity:
            init_component(world, e, 
                ECS_SIZEOF(ecs_entity_t), ECS_ALIGNOF(ecs_entity_t));
            init_type(world, e, EcsPrimitiveType);
            break;
        }
    }
}

static
void set_member(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    EcsMember *member = ecs_term(it, EcsMember, 1);

    int i, count = it->count;
    for (i = 0; i < count; i ++) {
        ecs_entity_t e = it->entities[i];
        ecs_entity_t parent = ecs_get_object(world, e, EcsChildOf, 0);
        if (!parent) {
            ecs_err("missing parent for member '%s'", ecs_get_name(world, e));
            continue;
        }

        add_member_to_struct(world, parent, e, member);
    }
}

static
void add_enum(ecs_iter_t *it) {
    ecs_world_t *world = it->world;

    int i, count = it->count;
    for (i = 0; i < count; i ++) {
        ecs_entity_t e = it->entities[i];
        
        if (init_component(
            world, e, ECS_SIZEOF(ecs_i32_t), ECS_ALIGNOF(ecs_i32_t)))
        {
            continue;
        }

        if (init_type(world, e, EcsEnumType)) {
            continue;
        }
    }
}

static
void add_bitmask(ecs_iter_t *it) {
    ecs_world_t *world = it->world;

    int i, count = it->count;
    for (i = 0; i < count; i ++) {
        ecs_entity_t e = it->entities[i];
        
        if (init_component(
            world, e, ECS_SIZEOF(ecs_u32_t), ECS_ALIGNOF(ecs_u32_t)))
        {
            continue;
        }

        if (init_type(world, e, EcsBitmaskType)) {
            continue;
        }
    }
}

static
void add_constant(ecs_iter_t *it) {
    ecs_world_t *world = it->world;

    int i, count = it->count;
    for (i = 0; i < count; i ++) {
        ecs_entity_t e = it->entities[i];
        ecs_entity_t parent = ecs_get_object(world, e, EcsChildOf, 0);
        if (!parent) {
            ecs_err("missing parent for constant '%s'", ecs_get_name(world, e));
            continue;
        }

        if (ecs_has(world, parent, EcsEnum)) {
            add_constant_to_enum(world, parent, e, it->event_id);
        } else if (ecs_has(world, parent, EcsBitmask)) {
            add_constant_to_bitmask(world, parent, e, it->event_id);
        }
    }
}

static
void set_array(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    EcsArray *array = ecs_term(it, EcsArray, 1);

    int i, count = it->count;
    for (i = 0; i < count; i ++) {
        ecs_entity_t e = it->entities[i];
        ecs_entity_t elem_type = array[i].type;
        int32_t elem_count = array[i].count;

        if (!elem_type) {
            ecs_err("array '%s' has no element type", ecs_get_name(world, e));
            continue;
        }

        if (!elem_count) {
            ecs_err("array '%s' has size 0", ecs_get_name(world, e));
            continue;
        }

        const EcsComponent *elem_ptr = ecs_get(world, elem_type, EcsComponent);
        if (init_component(
            world, e, elem_ptr->size * elem_count, elem_ptr->alignment))
        {
            continue;
        }

        if (init_type(world, e, EcsArrayType)) {
            continue;
        }
    }
}

static
void set_vector(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    EcsVector *array = ecs_term(it, EcsVector, 1);

    int i, count = it->count;
    for (i = 0; i < count; i ++) {
        ecs_entity_t e = it->entities[i];
        ecs_entity_t elem_type = array[i].type;

        if (!elem_type) {
            ecs_err("vector '%s' has no element type", ecs_get_name(world, e));
            continue;
        }

        if (init_component(world, e, 
            ECS_SIZEOF(ecs_vector_t*), ECS_ALIGNOF(ecs_vector_t*)))
        {
            continue;
        }

        if (init_type(world, e, EcsVectorType)) {
            continue;
        }
    }
}

static
void ecs_meta_type_init_default_ctor(ecs_iter_t *it) {
    ecs_world_t *world = it->world;

    int i;
    for (i = 0; i < it->count; i ++) {
        ecs_entity_t type = it->entities[i];

        /* If component has no component actions (which is typical if a type is
         * created with reflection data) make sure its values are always 
         * initialized with zero. This prevents the injection of invalid data 
         * through generic APIs after adding a component without setting it. */
        if (!ecs_component_has_actions(world, type)) {
            ecs_set_component_actions_w_id(world, type, 
                &(EcsComponentLifecycle){ 
                    .ctor = ecs_default_ctor
                });
        }
    }
}

static
void member_on_set(ecs_iter_t *it) {
    EcsMember *mbr = it->ptrs[0];
    if (!mbr->count) {
        mbr->count = 1;
    }
}

void FlecsMetaImport(
    ecs_world_t *world)
{
    ECS_MODULE(world, FlecsMeta);

    ecs_set_name_prefix(world, "Ecs");

    flecs_bootstrap_component(world, EcsMetaType);
    flecs_bootstrap_component(world, EcsMetaTypeSerialized);
    flecs_bootstrap_component(world, EcsPrimitive);
    flecs_bootstrap_component(world, EcsEnum);
    flecs_bootstrap_component(world, EcsBitmask);
    flecs_bootstrap_component(world, EcsMember);
    flecs_bootstrap_component(world, EcsStruct);
    flecs_bootstrap_component(world, EcsArray);
    flecs_bootstrap_component(world, EcsVector);

    flecs_bootstrap_tag(world, EcsConstant);

    ecs_set_component_actions(world, EcsMetaType, { .ctor = ecs_default_ctor });

    ecs_set_component_actions(world, EcsMetaTypeSerialized, { 
        .ctor = ecs_default_ctor,
        .move = ecs_move(EcsMetaTypeSerialized),
        .copy = ecs_copy(EcsMetaTypeSerialized),
        .dtor = ecs_dtor(EcsMetaTypeSerialized)
    });

    ecs_set_component_actions(world, EcsStruct, { 
        .ctor = ecs_default_ctor,
        .move = ecs_move(EcsStruct),
        .copy = ecs_copy(EcsStruct),
        .dtor = ecs_dtor(EcsStruct)
    });

    ecs_set_component_actions(world, EcsMember, { 
        .ctor = ecs_default_ctor,
        .on_set = member_on_set
    });

    ecs_set_component_actions(world, EcsEnum, { 
        .ctor = ecs_default_ctor,
        .move = ecs_move(EcsEnum),
        .copy = ecs_copy(EcsEnum),
        .dtor = ecs_dtor(EcsEnum)
    });

    ecs_set_component_actions(world, EcsBitmask, { 
        .ctor = ecs_default_ctor,
        .move = ecs_move(EcsBitmask),
        .copy = ecs_copy(EcsBitmask),
        .dtor = ecs_dtor(EcsBitmask)
    });

    /* Register triggers to finalize type information from component data */
    ecs_trigger_init(world, &(ecs_trigger_desc_t) {
        .term.id = ecs_id(EcsPrimitive),
        .term.subj.set.mask = EcsSelf,
        .events = {EcsOnSet},
        .callback = set_primitive
    });

    ecs_trigger_init(world, &(ecs_trigger_desc_t) {
        .term.id = ecs_id(EcsMember),
        .term.subj.set.mask = EcsSelf,
        .events = {EcsOnSet},
        .callback = set_member
    });

    ecs_trigger_init(world, &(ecs_trigger_desc_t) {
        .term.id = ecs_id(EcsEnum),
        .term.subj.set.mask = EcsSelf,
        .events = {EcsOnAdd},
        .callback = add_enum
    });

    ecs_trigger_init(world, &(ecs_trigger_desc_t) {
        .term.id = ecs_id(EcsBitmask),
        .term.subj.set.mask = EcsSelf,
        .events = {EcsOnAdd},
        .callback = add_bitmask
    });

    ecs_trigger_init(world, &(ecs_trigger_desc_t) {
        .term.id = EcsConstant,
        .term.subj.set.mask = EcsSelf,
        .events = {EcsOnAdd},
        .callback = add_constant
    });

    ecs_trigger_init(world, &(ecs_trigger_desc_t) {
        .term.id = ecs_pair(EcsConstant, EcsWildcard),
        .term.subj.set.mask = EcsSelf,
        .events = {EcsOnSet},
        .callback = add_constant
    });

    ecs_trigger_init(world, &(ecs_trigger_desc_t) {
        .term.id = ecs_id(EcsArray),
        .term.subj.set.mask = EcsSelf,
        .events = {EcsOnSet},
        .callback = set_array
    });

    ecs_trigger_init(world, &(ecs_trigger_desc_t) {
        .term.id = ecs_id(EcsVector),
        .term.subj.set.mask = EcsSelf,
        .events = {EcsOnSet},
        .callback = set_vector
    });

    ecs_trigger_init(world, &(ecs_trigger_desc_t) {
        .term.id = ecs_id(EcsMetaType),
        .term.subj.set.mask = EcsSelf,
        .events = {EcsOnSet},
        .callback = ecs_meta_type_serialized_init
    });

    ecs_trigger_init(world, &(ecs_trigger_desc_t) {
        .term.id = ecs_id(EcsMetaType),
        .term.subj.set.mask = EcsSelf,
        .events = {EcsOnSet},
        .callback = ecs_meta_type_init_default_ctor
    });

    /* Initialize primitive types */
    #define ECS_PRIMITIVE(world, type, primitive_kind)\
        ecs_entity_init(world, &(ecs_entity_desc_t) {\
            .entity = ecs_id(ecs_##type##_t),\
            .name = #type });\
        ecs_set(world, ecs_id(ecs_##type##_t), EcsPrimitive, {\
            .kind = primitive_kind\
        });

    ECS_PRIMITIVE(world, bool, EcsBool);
    ECS_PRIMITIVE(world, char, EcsChar);
    ECS_PRIMITIVE(world, byte, EcsByte);
    ECS_PRIMITIVE(world, u8, EcsU8);
    ECS_PRIMITIVE(world, u16, EcsU16);
    ECS_PRIMITIVE(world, u32, EcsU32);
    ECS_PRIMITIVE(world, u64, EcsU64);
    ECS_PRIMITIVE(world, uptr, EcsUPtr);
    ECS_PRIMITIVE(world, i8, EcsI8);
    ECS_PRIMITIVE(world, i16, EcsI16);
    ECS_PRIMITIVE(world, i32, EcsI32);
    ECS_PRIMITIVE(world, i64, EcsI64);
    ECS_PRIMITIVE(world, iptr, EcsIPtr);
    ECS_PRIMITIVE(world, f32, EcsF32);
    ECS_PRIMITIVE(world, f64, EcsF64);
    ECS_PRIMITIVE(world, string, EcsString);
    ECS_PRIMITIVE(world, entity, EcsEntity);

    #undef ECS_PRIMITIVE

    /* Set default child components */
    ecs_add_pair(world, ecs_id(EcsStruct), 
        EcsDefaultChildComponent, ecs_id(EcsMember));

    ecs_add_pair(world, ecs_id(EcsMember), 
        EcsDefaultChildComponent, ecs_id(EcsMember));

    ecs_add_pair(world, ecs_id(EcsEnum), 
        EcsDefaultChildComponent, EcsConstant);

    ecs_add_pair(world, ecs_id(EcsBitmask), 
        EcsDefaultChildComponent, EcsConstant);

    /* Initialize reflection data for meta components */
    ecs_entity_t type_kind = ecs_enum_init(world, &(ecs_enum_desc_t) {
        .entity.name = "TypeKind",
        .constants = {
            {.name = "PrimitiveType"},
            {.name = "BitmaskType"},
            {.name = "EnumType"},
            {.name = "StructType"},
            {.name = "ArrayType"},
            {.name = "VectorType"}
        }
    });

    ecs_struct_init(world, &(ecs_struct_desc_t) {
        .entity.entity = ecs_id(EcsMetaType),
        .members = {
            {.name = (char*)"kind", .type = type_kind}
        }
    });

    ecs_entity_t primitive_kind = ecs_enum_init(world, &(ecs_enum_desc_t) {
        .entity.name = "PrimitiveKind",
        .constants = {
            {.name = "Bool", 1}, 
            {.name = "Char"}, 
            {.name = "Byte"}, 
            {.name = "U8"}, 
            {.name = "U16"}, 
            {.name = "U32"}, 
            {.name = "U64"},
            {.name = "I8"}, 
            {.name = "I16"}, 
            {.name = "I32"}, 
            {.name = "I64"}, 
            {.name = "F32"}, 
            {.name = "F64"}, 
            {.name = "UPtr"},
            {.name = "IPtr"}, 
            {.name = "String"}, 
            {.name = "Entity"}
        }
    });

    ecs_struct_init(world, &(ecs_struct_desc_t) {
        .entity.entity = ecs_id(EcsPrimitive),
        .members = {
            {.name = (char*)"kind", .type = primitive_kind}
        }
    });

    ecs_struct_init(world, &(ecs_struct_desc_t) {
        .entity.entity = ecs_id(EcsMember),
        .members = {
            {.name = (char*)"type", .type = ecs_id(ecs_entity_t)},
            {.name = (char*)"count", .type = ecs_id(ecs_i32_t)}
        }
    });

    ecs_struct_init(world, &(ecs_struct_desc_t) {
        .entity.entity = ecs_id(EcsArray),
        .members = {
            {.name = (char*)"type", .type = ecs_id(ecs_entity_t)},
            {.name = (char*)"count", .type = ecs_id(ecs_i32_t)},
        }
    });

    ecs_struct_init(world, &(ecs_struct_desc_t) {
        .entity.entity = ecs_id(EcsVector),
        .members = {
            {.name = (char*)"type", .type = ecs_id(ecs_entity_t)}
        }
    });
}

#endif
