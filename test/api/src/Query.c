#include <api.h>

void Query_query_changed_after_new() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    
    ecs_new(world, Position);

    ecs_query_t *q = ecs_query_new(world, "[in] Position");
    test_assert(q != NULL);
    test_assert(ecs_query_changed(q, 0) == true);
    test_assert(ecs_query_changed(q, 0) == true);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == true);
    while (ecs_query_next(&it)) { }
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_new(world, Position);
    test_assert(ecs_query_changed(q, 0) == true);

    it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == true);

    while (ecs_query_next(&it)) { }
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_fini(world);
}

void Query_query_changed_after_delete() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    
    ecs_entity_t e1 = ecs_new(world, Position);

    ecs_query_t *q = ecs_query_new(world, "[in] Position");
    test_assert(q != NULL);
    test_assert(ecs_query_changed(q, 0) == true);
    test_assert(ecs_query_changed(q, 0) == true);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == true);

    while (ecs_query_next(&it)) { }
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_delete(world, e1);
    test_assert(ecs_query_changed(q, 0) == true);

    ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_fini(world);
}

void Query_query_changed_after_add() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    
    ecs_entity_t e1 = ecs_new(world, 0);

    ecs_query_t *q = ecs_query_new(world, "[in] Position");
    test_assert(q != NULL);
    test_assert(ecs_query_changed(q, 0) == true);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == false);
    while (ecs_query_next(&it)) { }
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_add(world, e1, Position);
    test_assert(ecs_query_changed(q, 0) == true);

    it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == true);
    while (ecs_query_next(&it)) { }
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_fini(world);
}

void Query_query_changed_after_remove() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    
    ecs_entity_t e1 = ecs_new(world, Position);

    ecs_query_t *q = ecs_query_new(world, "[in] Position");
    test_assert(q != NULL);
    test_assert(ecs_query_changed(q, 0) == true);
    test_assert(ecs_query_changed(q, 0) == true);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == true);
    while (ecs_query_next(&it)) { }
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_remove(world, e1, Position);
    test_assert(ecs_query_changed(q, 0) == true);

    it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == false);
    while (ecs_query_next(&it)) { }
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_fini(world);
}

void Query_query_changed_after_set() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    
    ecs_entity_t e1 = ecs_new(world, Position);

    ecs_query_t *q = ecs_query_new(world, "[in] Position");
    test_assert(q != NULL);
    test_assert(ecs_query_changed(q, 0) == true);
    test_assert(ecs_query_changed(q, 0) == true);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == true);
    while (ecs_query_next(&it)) { }
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_set(world, e1, Position, {10, 20});
    test_assert(ecs_query_changed(q, 0) == true);

    it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == true);
    while (ecs_query_next(&it)) { }
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_fini(world);
}

void Query_query_change_after_modified() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    
    ecs_entity_t e1 = ecs_new(world, Position);

    ecs_query_t *q = ecs_query_new(world, "[in] Position");
    test_assert(q != NULL);
    test_assert(ecs_query_changed(q, 0) == true);
    test_assert(ecs_query_changed(q, 0) == true);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == true);
    while (ecs_query_next(&it)) { }
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_modified(world, e1, Position);
    test_assert(ecs_query_changed(q, 0) == true);

    it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == true);
    while (ecs_query_next(&it)) { }
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_fini(world);
}

void Sys(ecs_iter_t *it) { }

void Query_query_change_after_out_system() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    ECS_SYSTEM(world, Sys, EcsOnUpdate, [out] Position);
    
    ecs_new(world, Position);

    ecs_query_t *q = ecs_query_new(world, "[in] Position");
    test_assert(q != NULL);
    test_assert(ecs_query_changed(q, 0) == true);
    test_assert(ecs_query_changed(q, 0) == true);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == true);
    while (ecs_query_next(&it)) { }
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_progress(world, 0);
    test_assert(ecs_query_changed(q, 0) == true);

    it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == true);
    while (ecs_query_next(&it)) { }
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_fini(world);
}

void Query_query_change_after_in_system() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    ECS_SYSTEM(world, Sys, EcsOnUpdate, [in] Position);
    
    ecs_new(world, Position);

    ecs_query_t *q = ecs_query_new(world, "[in] Position");
    test_assert(q != NULL);
    test_assert(ecs_query_changed(q, 0) == true);
    test_assert(ecs_query_changed(q, 0) == true);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == true);
    while (ecs_query_next(&it)) { }
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_progress(world, 0);
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_fini(world);
}

void Query_query_change_after_modified_out_term() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    
    ecs_entity_t e = ecs_new(world, Position);
    ecs_add(world, e, Velocity);

    ecs_query_t *q = ecs_query_new(world, "[in] Position, [out] Velocity");
    test_assert(q != NULL);
    test_assert(ecs_query_changed(q, 0) == true);
    test_assert(ecs_query_changed(q, 0) == true);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == true);
    while (ecs_query_next(&it)) { }
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_set(world, e, Position, {10, 20});
    test_assert(ecs_query_changed(q, 0) == true);

    it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == true);
    while (ecs_query_next(&it)) { }
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_set(world, e, Velocity, {1, 2});
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_fini(world);
}

void Query_query_change_check_iter() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_TAG(world, TagA);
    ECS_TAG(world, TagB);
    ECS_TAG(world, TagC);
    
    ecs_entity_t e1 = ecs_new(world, Position);
    ecs_entity_t e2 = ecs_new(world, Position);
    ecs_entity_t e3 = ecs_new(world, Position);

    ecs_add(world, e1, TagA);
    ecs_add(world, e2, TagB);
    ecs_add(world, e3, TagC);

    ecs_query_t *q = ecs_query_new(world, "[in] Position");
    test_assert(q != NULL);
    test_assert(ecs_query_changed(q, 0) == true);
    test_assert(ecs_query_changed(q, 0) == true);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == true);
    while (ecs_query_next(&it)) { 
        test_bool(ecs_query_changed(q, &it), true);
    }
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_modified(world, e1, Position);
    test_assert(ecs_query_changed(q, 0) == true);

    it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == true);

    test_bool(ecs_query_next(&it), true);
    test_int(it.count, 1);
    test_int(it.entities[0], e1);
    test_bool(ecs_query_changed(q, &it), true);

    test_bool(ecs_query_next(&it), true);
    test_int(it.count, 1);
    test_int(it.entities[0], e2);
    test_bool(ecs_query_changed(q, &it), false);

    test_bool(ecs_query_next(&it), true);
    test_int(it.count, 1);
    test_int(it.entities[0], e3);
    test_bool(ecs_query_changed(q, &it), false);

    test_bool(ecs_query_changed(q, NULL), false);

    it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == false);
    while (ecs_query_next(&it)) { 
        test_bool(ecs_query_changed(q, &it), false);
    }
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_modified(world, e2, Position);

    it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == true);

    test_bool(ecs_query_next(&it), true);
    test_int(it.count, 1);
    test_int(it.entities[0], e1);
    test_bool(ecs_query_changed(q, &it), false);

    test_bool(ecs_query_next(&it), true);
    test_int(it.count, 1);
    test_int(it.entities[0], e2);
    test_bool(ecs_query_changed(q, &it), true);

    test_bool(ecs_query_next(&it), true);
    test_int(it.count, 1);
    test_int(it.entities[0], e3);
    test_bool(ecs_query_changed(q, &it), false);

    test_bool(ecs_query_changed(q, NULL), false);

    ecs_fini(world);
}

void Query_query_change_check_iter_after_skip_read() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    
    ecs_entity_t e = ecs_new(world, Position);

    ecs_query_t *q = ecs_query_new(world, "[in] Position");
    test_assert(q != NULL);
    test_assert(ecs_query_changed(q, 0) == true);
    test_assert(ecs_query_changed(q, 0) == true);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == true);
    while (ecs_query_next(&it)) { }
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_set(world, e, Position, {10, 20});
    test_assert(ecs_query_changed(q, 0) == true);

    it = ecs_query_iter(world, q);
    test_bool(ecs_query_next(&it), true);
    test_assert(ecs_query_changed(q, &it) == true);
    ecs_query_skip(&it);
    test_bool(ecs_query_next(&it), false);

    test_assert(ecs_query_changed(q, 0) == true);

    it = ecs_query_iter(world, q);
    test_bool(ecs_query_next(&it), true);
    test_assert(ecs_query_changed(q, &it) == true);
    test_bool(ecs_query_next(&it), false);
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_fini(world);
}

void Query_query_change_check_iter_after_skip_write() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    
    ecs_new(world, Position);

    ecs_query_t *qw = ecs_query_new(world, "[out] Position");

    ecs_query_t *q = ecs_query_new(world, "[in] Position");
    test_assert(q != NULL);
    test_assert(ecs_query_changed(q, 0) == true);
    test_assert(ecs_query_changed(q, 0) == true);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == true);
    while (ecs_query_next(&it)) { }
    test_assert(ecs_query_changed(q, 0) == false);

    it = ecs_query_iter(world, qw);
    test_assert(ecs_query_changed(q, 0) == false);
    test_bool(ecs_query_next(&it), true);
    test_assert(ecs_query_changed(q, 0) == false);
    ecs_query_skip(&it);
    test_bool(ecs_query_next(&it), false);
    test_assert(ecs_query_changed(q, 0) == false);

    test_assert(ecs_query_changed(q, 0) == false);

    it = ecs_query_iter(world, qw);
    test_assert(ecs_query_changed(q, 0) == false);
    test_bool(ecs_query_next(&it), true);
    test_assert(ecs_query_changed(q, 0) == false);
    test_bool(ecs_query_next(&it), false);
    test_assert(ecs_query_changed(q, 0) == true);

    it = ecs_query_iter(world, q);
    test_bool(ecs_query_next(&it), true);
    test_assert(ecs_query_changed(q, &it) == true);
    test_bool(ecs_query_next(&it), false);
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_fini(world);
}

void Query_query_change_parent_term() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    
    ecs_entity_t parent = ecs_new(world, Position);
    ecs_entity_t child = ecs_new(world, Position);
    ecs_add_pair(world, child, EcsChildOf, parent);

    ecs_query_t *q = ecs_query_new(world, "[in] Position, [in] Position(parent)");
    test_assert(q != NULL);
    test_assert(ecs_query_changed(q, 0) == true);
    test_assert(ecs_query_changed(q, 0) == true);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == true);
    while (ecs_query_next(&it)) { }
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_set(world, parent, Position, {10, 20});

    test_assert(ecs_query_changed(q, 0) == true);

    it = ecs_query_iter(world, q);
    test_bool(ecs_query_next(&it), true);
    test_assert(ecs_query_changed(q, &it) == true);
    test_bool(ecs_query_next(&it), false);
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_fini(world);
}

void Query_query_change_prefab_term() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    
    ecs_entity_t base = ecs_new(world, Position);
    ecs_new_w_pair(world, EcsIsA, base);

    ecs_query_t *q = ecs_query_new(world, "[in] Position(super)");
    test_assert(q != NULL);
    test_assert(ecs_query_changed(q, 0) == true);
    test_assert(ecs_query_changed(q, 0) == true);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_changed(q, 0) == true);
    while (ecs_query_next(&it)) { }
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_set(world, base, Position, {10, 20});

    test_assert(ecs_query_changed(q, 0) == true);

    it = ecs_query_iter(world, q);
    test_bool(ecs_query_next(&it), true);
    test_assert(ecs_query_changed(q, &it) == true);
    test_bool(ecs_query_next(&it), false);
    test_assert(ecs_query_changed(q, 0) == false);

    ecs_fini(world);
}

void Query_subquery_match_existing() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_TYPE(world, Type, Position, Velocity);

    ecs_bulk_new(world, Position, 3);
    ecs_bulk_new(world, Velocity, 3);
    const ecs_id_t *ids = ecs_bulk_new(world, Position, 3);
    ecs_add(world, ids[0], Velocity);
    ecs_add(world, ids[1], Velocity);
    ecs_add(world, ids[2], Velocity);

    ecs_query_t *q = ecs_query_new(world, "Position");
    test_assert(q != NULL);

    ecs_query_t *sq = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter.expr = "Velocity",
        .parent = q
    });
    
    test_assert(sq != NULL);

    int32_t table_count = 0, entity_count = 0;
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        table_count ++;
        entity_count += it.count;

        int32_t i;
        for (i = 0; i < it.count; i ++) {
            test_assert(ecs_has(world, it.entities[i], Position));
        }
    }

    test_int(table_count, 2);
    test_int(entity_count, 6);

    table_count = 0, entity_count = 0;
    it = ecs_query_iter(world, sq);
    while (ecs_query_next(&it)) {
        table_count ++;
        entity_count += it.count;

        int32_t i;
        for (i = 0; i < it.count; i ++) {
            test_assert(ecs_has(world, it.entities[i], Position));
            test_assert(ecs_has(world, it.entities[i], Velocity));
        }
    } 

    test_int(table_count, 1);
    test_int(entity_count, 3); 

     ecs_query_fini(sq); 

    ecs_fini(world);
}

void Query_subquery_match_new() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_query_t *q = ecs_query_new(world, "Position");
    test_assert(q != NULL);

    ecs_query_t *sq = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter.expr = "Velocity",
        .parent = q
    });
    test_assert(sq != NULL);

    ecs_bulk_new(world, Position, 3);
    ecs_bulk_new(world, Velocity, 3);
    const ecs_id_t *ids = ecs_bulk_new(world, Position, 3);
    ecs_add(world, ids[0], Velocity);
    ecs_add(world, ids[1], Velocity);
    ecs_add(world, ids[2], Velocity);

    int32_t table_count = 0, entity_count = 0;
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        table_count ++;
        entity_count += it.count;

        int32_t i;
        for (i = 0; i < it.count; i ++) {
            test_assert(ecs_has(world, it.entities[i], Position));
        }
    }

    test_int(table_count, 2);
    test_int(entity_count, 6);

    table_count = 0, entity_count = 0;
    it = ecs_query_iter(world, sq);
    while (ecs_query_next(&it)) {
        table_count ++;
        entity_count += it.count;

        int32_t i;
        for (i = 0; i < it.count; i ++) {
            test_assert(ecs_has(world, it.entities[i], Position));
            test_assert(ecs_has(world, it.entities[i], Velocity));
        }
    } 

    test_int(table_count, 1);
    test_int(entity_count, 3);  

     ecs_query_fini(sq);

    ecs_fini(world);
}

void Query_subquery_inactive() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_bulk_new(world, Position, 3);
    ecs_bulk_new(world, Velocity, 3);
    ECS_ENTITY(world, e, Position, Velocity);

    ecs_query_t *q = ecs_query_new(world, "Position");
    test_assert(q != NULL);

    ecs_query_t *sq = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter.expr = "Velocity",
        .parent = q
    });
    test_assert(sq != NULL);
    
    /* Create an empty table which should deactivate it for both queries */
    ecs_delete(world, e);

    int32_t table_count = 0, entity_count = 0;
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        table_count ++;
        entity_count += it.count;

        int32_t i;
        for (i = 0; i < it.count; i ++) {
            test_assert(ecs_has(world, it.entities[i], Position));
        }
    }

    test_int(table_count, 1);
    test_int(entity_count, 3);

    table_count = 0, entity_count = 0;
    it = ecs_query_iter(world, sq);
    test_int(it.table_count, 0);

    table_count = 0, entity_count = 0;
    it = ecs_query_iter(world, sq);
    while (ecs_query_next(&it)) {
        table_count ++;
    }

    test_int(table_count, 0);

     ecs_query_fini(sq);

    ecs_fini(world);
}

void Query_subquery_unmatch() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_entity_t parent = ecs_new(world, 0);
    ecs_add(world, parent, Position);

    ecs_entity_t e1 = ecs_new(world, 0);
    ecs_add(world, e1, Position);
    ecs_add(world, e1, Velocity);
    ecs_add_pair(world, e1, EcsChildOf, parent);

    ecs_entity_t e2 = ecs_new(world, 0);
    ecs_add(world, e2, Position);
    ecs_add_pair(world, e2, EcsChildOf, parent);

    ecs_query_t *q = ecs_query_new(world, "Position, Position(parent)");
    test_assert(q != NULL);

    ecs_query_t *sq = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter.expr = "Velocity",
        .parent = q
    });
    test_assert(sq != NULL);

    int32_t table_count = 0, entity_count = 0;
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        table_count ++;
        entity_count += it.count;

        int32_t i;
        for (i = 0; i < it.count; i ++) {
            test_assert(ecs_has(world, it.entities[i], Position));
        }
    }

    test_int(table_count, 2);  
    test_int(entity_count, 2);

    table_count = 0, entity_count = 0;
    it = ecs_query_iter(world, sq);
    while (ecs_query_next(&it)) {
        table_count ++;
        entity_count += it.count;

        int32_t i;
        for (i = 0; i < it.count; i ++) {
            test_assert(ecs_has(world, it.entities[i], Position));
            test_assert(ecs_has(world, it.entities[i], Velocity));
        }
    }

    test_int(table_count, 1);
    test_int(entity_count, 1);

    /* Query now no longer match */
    ecs_remove(world, parent, Position);

    /* Force rematching */
    ecs_progress(world, 0);

    /* Test if tables have been unmatched */
    it = ecs_query_iter(world, q);
    test_int(it.table_count, 0);

    it = ecs_query_iter(world, sq);
    test_int(it.table_count, 0);

     ecs_query_fini(sq);

    ecs_fini(world);
}

void Query_subquery_rematch() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_entity_t parent = ecs_new(world, 0);
    ecs_add(world, parent, Position);

    ecs_entity_t e1 = ecs_new(world, 0);
    ecs_add(world, e1, Position);
    ecs_add(world, e1, Velocity);
    ecs_add_pair(world, e1, EcsChildOf, parent);

    ecs_entity_t e2 = ecs_new(world, 0);
    ecs_add(world, e2, Position);
    ecs_add_pair(world, e2, EcsChildOf, parent);

    ecs_query_t *q = ecs_query_new(world, "Position, Position(parent)");
    test_assert(q != NULL);

    ecs_query_t *sq = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter.expr = "Velocity",
        .parent = q
    });
    test_assert(sq != NULL);

    int32_t table_count = 0, entity_count = 0;
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        table_count ++;
        entity_count += it.count;

        int32_t i;
        for (i = 0; i < it.count; i ++) {
            test_assert(ecs_has(world, it.entities[i], Position));
        }
    }

    test_int(table_count, 2);  
    test_int(entity_count, 2);

    table_count = 0, entity_count = 0;
    it = ecs_query_iter(world, sq);
    while (ecs_query_next(&it)) {
        table_count ++;
        entity_count += it.count;

        int32_t i;
        for (i = 0; i < it.count; i ++) {
            test_assert(ecs_has(world, it.entities[i], Position));
            test_assert(ecs_has(world, it.entities[i], Velocity));
        }
    }

    test_int(table_count, 1);
    test_int(entity_count, 1);

    /* Query now no longer match */
    ecs_remove(world, parent, Position);

    /* Force unmatching */
    ecs_progress(world, 0);

    /* Test if tables have been unmatched */
    it = ecs_query_iter(world, q);
    test_int(it.table_count, 0);

    it = ecs_query_iter(world, sq);
    test_int(it.table_count, 0);

    /* Rematch queries */
    ecs_add(world, parent, Position);    

    /* Force rematching */
    ecs_progress(world, 0);

    /* Test if tables have been rematched */
    it = ecs_query_iter(world, q);
    test_int(it.table_count, 2);

    it = ecs_query_iter(world, sq);
    test_int(it.table_count, 1);    

     ecs_query_fini(sq);

    ecs_fini(world);
}

void Query_subquery_rematch_w_parent_optional() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);

    ecs_entity_t parent = ecs_new(world, 0);

    ecs_entity_t e1 = ecs_new(world, 0);
    ecs_add(world, e1, Position);
    ecs_add(world, e1, Velocity);
    ecs_add_pair(world, e1, EcsChildOf, parent);

    ecs_entity_t e2 = ecs_new(world, 0);
    ecs_add(world, e2, Position);
    ecs_add_pair(world, e2, EcsChildOf, parent);

    ecs_query_t *q = ecs_query_new(world, "Position, ?Position(parent)");
    test_assert(q != NULL);

    ecs_query_t *sq = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter.expr = "Velocity",
        .parent = q
    });
    test_assert(sq != NULL);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_int(it.table_count, 2);

    it = ecs_query_iter(world, sq);
    test_int(it.table_count, 1);

    /* Trigger rematch */
    ecs_add(world, parent, Position);
    ecs_progress(world, 0);

    /* Tables should be matched */
    it = ecs_query_iter(world, q);
    test_int(it.table_count, 3);

    it = ecs_query_iter(world, sq);
    test_int(it.table_count, 1);

     ecs_query_fini(sq);

    ecs_fini(world);
}

void Query_subquery_rematch_w_sub_optional() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);

    ecs_entity_t parent = ecs_new(world, 0);
    // ecs_add(world, parent, Position);

    ecs_entity_t e1 = ecs_new(world, 0);
    ecs_add(world, e1, Position);
    ecs_add(world, e1, Velocity);
    ecs_add_pair(world, e1, EcsChildOf, parent);

    ecs_entity_t e2 = ecs_new(world, 0);
    ecs_add(world, e2, Position);
    ecs_add_pair(world, e2, EcsChildOf, parent);

    ecs_query_t *q = ecs_query_new(world, "Position, ?Position(parent)");
    test_assert(q != NULL);

    ecs_query_t *sq = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter.expr = "Velocity",
        .parent = q
    });
    test_assert(sq != NULL);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_int(it.table_count, 2);

    it = ecs_query_iter(world, sq);
    test_int(it.table_count, 1);

    /* Trigger rematch */
    ecs_add(world, parent, Position);
    ecs_progress(world, 0);

    /* Tables should be matched */
    it = ecs_query_iter(world, q);
    test_int(it.table_count, 3);

    it = ecs_query_iter(world, sq);
    test_int(it.table_count, 1);

     ecs_query_fini(sq);

    ecs_fini(world);
}

void Query_query_single_pairs() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_TAG(world, Rel);

    ECS_ENTITY(world, e1, (Rel, Position));
    ECS_ENTITY(world, e2, (Rel, Velocity));
    ECS_ENTITY(world, e3, Position);
    ECS_ENTITY(world, e4, Velocity);

    int32_t table_count = 0, entity_count = 0;

    ecs_query_t *q = ecs_query_new(world, "(Rel, Velocity)");
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        table_count ++;

        int32_t i;
        for (i = 0; i < it.count; i ++) {
            test_assert(it.entities[i] == e2);
            entity_count ++;
        }
    }

    test_int(table_count, 1);
    test_int(entity_count, 1);

    ecs_fini(world);
}

void Query_query_single_instanceof() {
    ecs_world_t *world = ecs_init();

    ECS_ENTITY(world, BaseA, 0);
    ECS_ENTITY(world, BaseB, 0);
    ECS_ENTITY(world, e1, (IsA, BaseB));
    ECS_ENTITY(world, e2, (IsA, BaseA));

    int32_t table_count = 0, entity_count = 0;

    ecs_query_t *q = ecs_query_new(world, "(IsA, BaseA)");
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        table_count ++;

        int32_t i;
        for (i = 0; i < it.count; i ++) {
            test_assert(it.entities[i] == e2);
            entity_count ++;
        }
    }

    test_int(table_count, 1);
    test_int(entity_count, 1);

    ecs_fini(world);
}

void Query_query_single_childof() {
    ecs_world_t *world = ecs_init();

    ECS_ENTITY(world, BaseA, 0);
    ECS_ENTITY(world, BaseB, 0);
    ECS_ENTITY(world, e1, (ChildOf, BaseB));
    ECS_ENTITY(world, e2, (ChildOf, BaseA));

    int32_t table_count = 0, entity_count = 0;

    ecs_query_t *q = ecs_query_new(world, "(ChildOf, BaseA)");
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        table_count ++;

        int32_t i;
        for (i = 0; i < it.count; i ++) {
            test_assert(it.entities[i] == e2);
            entity_count ++;
        }
    }

    test_int(table_count, 1);
    test_int(entity_count, 1);

    ecs_fini(world);
}

void Query_query_optional_owned() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_entity_t base = ecs_new(world, Velocity);
    
    ecs_entity_t e1 = ecs_new(world, Position);
    ecs_add_pair(world, e1, EcsIsA, base);

    ecs_entity_t e2 = ecs_new(world, Position);
    ecs_add(world, e2, Velocity);

    ecs_entity_t e3 = ecs_new(world, Position);

    ecs_query_t *q = ecs_query_new(world, "Position, ?Velocity(self)");
    test_assert(q != NULL);

    ecs_iter_t it = ecs_query_iter(world, q);
    int32_t count = 0;
    
    while (ecs_query_next(&it)) {
        Position *p = ecs_term(&it, Position, 1);
        Velocity *v = ecs_term(&it, Velocity, 2);
        
        test_int(it.count, 1);
        test_assert(p != NULL);

        if (it.entities[0] == e1) {
            test_assert(v == NULL);
            test_bool(ecs_term_is_set(&it, 1), true);
            test_bool(ecs_term_is_set(&it, 2), false); 
        } else if (it.entities[0] == e2) {
            test_assert(v != NULL);
            test_bool(ecs_term_is_set(&it, 1), true);
            test_bool(ecs_term_is_set(&it, 2), true); 
        } else if (it.entities[0] == e3) {
            test_assert(v == NULL);
            test_bool(ecs_term_is_set(&it, 1), true);
            test_bool(ecs_term_is_set(&it, 2), false);
        }

        count ++;
    }

    test_int(count, 3);

    ecs_fini(world);
}

void Query_query_optional_shared() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_entity_t base = ecs_new(world, Velocity);
    
    ecs_entity_t e1 = ecs_new(world, Position);
    ecs_add_pair(world, e1, EcsIsA, base);

    ecs_entity_t e2 = ecs_new(world, Position);
    ecs_add(world, e2, Velocity);

    ecs_entity_t e3 = ecs_new(world, Position);

    ecs_query_t *q = ecs_query_new(world, "Position, ?Velocity(super)");
    test_assert(q != NULL);

    ecs_iter_t it = ecs_query_iter(world, q);
    int32_t count = 0;
    
    while (ecs_query_next(&it)) {
        Position *p = ecs_term(&it, Position, 1);
        Velocity *v = ecs_term(&it, Velocity, 2);
        
        test_int(it.count, 1);
        test_assert(p != NULL);
        
        if (it.entities[0] == e1) {
            test_assert(v != NULL);
            test_bool(ecs_term_is_set(&it, 1), true);
            test_bool(ecs_term_is_set(&it, 2), true);
        } else if (it.entities[0] == e2) {
            test_assert(v == NULL);
            test_bool(ecs_term_is_set(&it, 1), true);
            test_bool(ecs_term_is_set(&it, 2), false);            
        } else if (it.entities[0] == e3) {
            test_assert(v == NULL);
            test_bool(ecs_term_is_set(&it, 1), true);
            test_bool(ecs_term_is_set(&it, 2), false); 
        }

        count ++;
    }

    test_int(count, 3);

    ecs_fini(world);
}

void Query_query_optional_shared_nested() {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_entity_t base_base = ecs_new(world, Velocity);
    ecs_entity_t base = ecs_new(world, 0);
    ecs_add_pair(world, base, EcsIsA, base_base);
    
    ecs_entity_t e1 = ecs_new(world, Position);
    ecs_add_pair(world, e1, EcsIsA, base);

    ecs_entity_t e2 = ecs_new(world, Position);
    ecs_add(world, e2, Velocity);

    ecs_entity_t e3 = ecs_new(world, Position);

    ecs_query_t *q = ecs_query_new(world, "Position, ?Velocity(super)");
    test_assert(q != NULL);

    ecs_iter_t it = ecs_query_iter(world, q);
    int32_t count = 0;
    
    while (ecs_query_next(&it)) {
        Position *p = ecs_term(&it, Position, 1);
        Velocity *v = ecs_term(&it, Velocity, 2);
        
        test_int(it.count, 1);
        test_assert(p != NULL);
        
        if (it.entities[0] == e1) {
            test_assert(v != NULL);
        } else if (it.entities[0] == e2) {
            test_assert(v == NULL);
        } else if (it.entities[0] == e3) {
            test_assert(v == NULL);
        }

        count ++;
    }

    test_int(count, 3);

    ecs_fini(world);
}

void Query_query_optional_any() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_entity_t base = ecs_new(world, Velocity);
    
    ecs_entity_t e1 = ecs_new(world, Position);
    ecs_add_pair(world, e1, EcsIsA, base);

    ecs_entity_t e2 = ecs_new(world, Position);
    ecs_add(world, e2, Velocity);

    ecs_entity_t e3 = ecs_new(world, Position);

    ecs_query_t *q = ecs_query_new(world, "Position, ?Velocity(self|super)");
    test_assert(q != NULL);

    ecs_iter_t it = ecs_query_iter(world, q);
    int32_t count = 0;
    
    while (ecs_query_next(&it)) {
        Position *p = ecs_term(&it, Position, 1);
        Velocity *v = ecs_term(&it, Velocity, 2);
        
        test_int(it.count, 1);
        test_assert(p != NULL);
        
        if (it.entities[0] == e1) {
            test_assert(v != NULL);
            test_assert(!ecs_term_is_owned(&it, 2));
        } else if (it.entities[0] == e2) {
            test_assert(v != NULL);
            test_assert(ecs_term_is_owned(&it, 2));
        } else if (it.entities[0] == e3) {
            test_assert(v == NULL);
        }

        count ++;
    }

    test_int(count, 3);

    ecs_fini(world);
}

void Query_query_rematch_optional_after_add() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_entity_t base = ecs_new(world, 0);
    
    ecs_entity_t e1 = ecs_new(world, Position);
    ecs_add_pair(world, e1, EcsIsA, base);
    ecs_entity_t e2 = ecs_new(world, Position);
    ecs_add(world, e2, Velocity);
    ecs_entity_t e3 = ecs_new(world, Position);

    ecs_query_t *q = ecs_query_new(world, "Position, ?Velocity(super)");
    test_assert(q != NULL);

    /* First iteration, base doesn't have Velocity but query should match with
     * entity anyway since the component is optional */
    ecs_iter_t it = ecs_query_iter(world, q);
    int32_t count = 0;
    
    while (ecs_query_next(&it)) {
        Position *p = ecs_term(&it, Position, 1);
        Velocity *v = ecs_term(&it, Velocity, 2);
        
        test_int(it.count, 1);
        test_assert(p != NULL);
        
        if (it.entities[0] == e1) {
            test_assert(v == NULL);
        } else if (it.entities[0] == e2) {
            test_assert(v == NULL);
        } else if (it.entities[0] == e3) {
            test_assert(v == NULL);
        }

        count ++;
    }
    test_int(count, 3);

    /* Add Velocity to base, should trigger rematch */
    ecs_add(world, base, Velocity);

    /* Trigger a merge, which triggers the rematch */
    ecs_staging_begin(world);
    ecs_staging_end(world);

    /* Second iteration, base has Velocity and entity should be able to access
     * the shared component. */
    it = ecs_query_iter(world, q);
    count = 0;
    
    while (ecs_query_next(&it)) {
        Position *p = ecs_term(&it, Position, 1);
        Velocity *v = ecs_term(&it, Velocity, 2);
        
        test_int(it.count, 1);
        test_assert(p != NULL);
        
        if (it.entities[0] == e1) {
            test_assert(v != NULL);
        } else if (it.entities[0] == e2) {
            test_assert(v == NULL);
        } else if (it.entities[0] == e3) {
            test_assert(v == NULL);
        }

        count ++;
    }
    test_int(count, 3);    

    ecs_fini(world);    
}

void Query_get_owned_tag() {
    ecs_world_t *world = ecs_init();

    ECS_TAG(world, Tag);

    ecs_entity_t e = ecs_new(world, Tag);

    ecs_query_t *q = ecs_query_new(world, "Tag");

    int count = 0;
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        test_assert(ecs_term_w_size(&it, 0, 1) == NULL);
        test_int(it.count, 1);
        test_int(it.entities[0], e);
        count += it.count;
    }

    test_int(count, 1);

    ecs_fini(world);
}

void Query_get_shared_tag() {
    ecs_world_t *world = ecs_init();

    ECS_TAG(world, Tag);

    ecs_entity_t base = ecs_new(world, Tag);
    ecs_entity_t instance = ecs_new_w_pair(world, EcsIsA, base);

    ecs_query_t *q = ecs_query_new(world, "Tag(super)");

    int count = 0;
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        test_assert(ecs_term_w_size(&it, 0, 1) == NULL);
        test_int(it.count, 1);
        test_int(it.entities[0], instance);
        count += it.count;
    }

    test_int(count, 1);

    ecs_fini(world);
}

void Query_explicit_delete() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    ecs_query_t *q = ecs_query_new(world, "Position");
    test_assert(q != NULL);

    /* Ensure query isn't deleted twice when deleting world */
     ecs_query_fini(q);

    ecs_fini(world);
}

void Query_get_column_size() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    
    ecs_new(world, Position);

    ecs_query_t *q = ecs_query_new(world, "Position");
    test_assert(q != NULL);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_next(&it));
    test_int(ecs_term_size(&it, 1), sizeof(Position));

    ecs_fini(world);    
}

void Query_orphaned_query() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    
    ecs_new(world, Position);

    ecs_query_t *q = ecs_query_new(world, "Position");
    test_assert(q != NULL);

    /* Nonsense subquery, doesn't matter, this is just for orphan testing */
    ecs_query_t *sq = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter.expr = "Position",
        .parent = q
    });
    test_assert(sq != NULL);

    test_assert(!ecs_query_orphaned(sq));

     ecs_query_fini(q);

    test_assert(ecs_query_orphaned(sq));
    
     ecs_query_fini(sq);

    ecs_fini(world);
}

void Query_nested_orphaned_query() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    
    ecs_new(world, Position);

    ecs_query_t *q = ecs_query_new(world, "Position");
    test_assert(q != NULL);

    /* Nonsense subquery, doesn't matter, this is just for orphan testing */
    ecs_query_t *sq = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter.expr = "Position",
        .parent = q
    });
    test_assert(sq != NULL);

    ecs_query_t *nsq = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter.expr = "Position",
        .parent = sq
    });
    test_assert(nsq != NULL);    

    test_assert(!ecs_query_orphaned(sq));
    test_assert(!ecs_query_orphaned(nsq));

     ecs_query_fini(q);

    test_assert(ecs_query_orphaned(sq));
    test_assert(ecs_query_orphaned(nsq));
    
     ecs_query_fini(sq);
     ecs_query_fini(nsq);

    ecs_fini(world);
}

void Query_invalid_access_orphaned_query() {
    install_test_abort();

    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    
    ecs_new(world, Position);

    ecs_query_t *q = ecs_query_new(world, "Position");
    test_assert(q != NULL);

    /* Nonsense subquery, doesn't matter, this is just for orphan testing */
    ecs_query_t *sq = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter.expr = "Position",
        .parent = q
    });
    test_assert(sq != NULL);

    test_assert(!ecs_query_orphaned(sq));

     ecs_query_fini(q);

    test_expect_abort();

    ecs_query_iter(world, sq);  
}

void Query_stresstest_query_free() {
    ecs_world_t *world = ecs_init();

    ECS_TAG(world, Foo);
    ECS_TAG(world, Bar);
    ECS_TAG(world, Hello);

    ecs_entity_t e = ecs_new_id(world);
    ecs_add(world, e, Foo);
    ecs_add(world, e, Bar);
    ecs_add(world, e, Hello);

    /* Create & delete query to test if query is properly unregistered with
     * the table */

    for (int i = 0; i < 10000; i ++) {
        ecs_query_t *q = ecs_query_new(world, "Foo");
         ecs_query_fini(q);
    }

    /* If code did not crash, test passes */
    test_assert(true);

    ecs_fini(world);
}

void Query_only_from_entity() {
    ecs_world_t *world = ecs_init();

    ECS_TAG(world, Tag);

    ecs_entity_t e = ecs_set_name(world, 0, "e");

    ecs_query_t *q = ecs_query_new(world, "Tag(e)");
    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(!ecs_query_next(&it));

    ecs_add(world, e, Tag);

    it = ecs_query_iter(world, q);
    test_assert(ecs_query_next(&it));
    test_assert(ecs_term_source(&it, 1) == e);
    test_assert(ecs_term_id(&it, 1) == Tag);

    ecs_fini(world);
}

void Query_only_from_singleton() {
    ecs_world_t *world = ecs_init();

    ECS_TAG(world, Tag);

    ecs_entity_t e = ecs_set_name(world, 0, "e");

    ecs_query_t *q = ecs_query_new(world, "$e");
    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(!ecs_query_next(&it));

    ecs_add_id(world, e, e);

    it = ecs_query_iter(world, q);
    test_assert(ecs_query_next(&it));
    test_assert(ecs_term_source(&it, 1) == e);
    test_assert(ecs_term_id(&it, 1) == e);

    ecs_fini(world);
}

void Query_only_not_from_entity() {
    ecs_world_t *world = ecs_init();

    ECS_TAG(world, Tag);

    ecs_entity_t e = ecs_set_name(world, 0, "e");

    ecs_query_t *q = ecs_query_new(world, "!Tag(e)");
    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_next(&it));
    test_assert(ecs_term_source(&it, 1) == e);
    test_assert(ecs_term_id(&it, 1) == Tag);

    ecs_add(world, e, Tag);

    it = ecs_query_iter(world, q);
    test_assert(!ecs_query_next(&it));

    ecs_fini(world);
}

void Query_only_not_from_singleton() {
    ecs_world_t *world = ecs_init();

    ECS_TAG(world, Tag);

    ecs_entity_t e = ecs_set_name(world, 0, "e");

    ecs_query_t *q = ecs_query_new(world, "!$e");
    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_next(&it));
    test_assert(ecs_term_source(&it, 1) == e);
    test_assert(ecs_term_id(&it, 1) == e);

    ecs_add_id(world, e, e);

    it = ecs_query_iter(world, q);
    test_assert(!ecs_query_next(&it));

    ecs_fini(world);
}

void Query_get_filter() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_query_t *q = ecs_query_new(world, "Position, Velocity");
    test_assert(q != NULL);

    const ecs_filter_t *f = ecs_query_get_filter(q);
    test_assert(f != NULL);

    test_int(f->term_count, 2);
    test_int(f->terms[0].id, ecs_id(Position));
    test_int(f->terms[1].id, ecs_id(Velocity));

    ecs_fini(world);
}

static
uint64_t group_by_first_id(
    ecs_world_t *world,
    ecs_type_t type,
    ecs_id_t id,
    void *ctx)
{
    ecs_id_t *first = ecs_vector_first(type, ecs_id_t);
    if (!first) {
        return 0;
    }

    return first[0];
}

int order_by_entity(
    ecs_entity_t e1,
    const void *ptr1,
    ecs_entity_t e2,
    const void *ptr2)
{
    return (e1 > e2) - (e1 < e2);
}

void Query_group_by() {
    ecs_world_t *world = ecs_init();

    ECS_TAG(world, TagA);
    ECS_TAG(world, TagB);
    ECS_TAG(world, TagC);
    ECS_TAG(world, TagX);

    ecs_query_t *q = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter.terms = {{TagX}},
        .group_by = group_by_first_id
    });

    ecs_entity_t e1 = ecs_new(world, TagX);
    ecs_entity_t e2 = ecs_new(world, TagX);
    ecs_entity_t e3 = ecs_new(world, TagX);

    ecs_add_id(world, e1, TagC);
    ecs_add_id(world, e2, TagB);
    ecs_add_id(world, e3, TagA);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_bool(ecs_query_next(&it), true);
    test_int(it.count, 1);
    test_int(it.entities[0], e3);

    test_bool(ecs_query_next(&it), true);
    test_int(it.count, 1);
    test_int(it.entities[0], e2);

    test_bool(ecs_query_next(&it), true);
    test_int(it.count, 1);
    test_int(it.entities[0], e1);    

    test_bool(ecs_query_next(&it), false);

    ecs_fini(world);
}

static int ctx_value;
static int ctx_value_free_invoked = 0;
static
void ctx_value_free(void *ctx) {
    test_assert(ctx == &ctx_value);
    ctx_value_free_invoked ++;
}

void Query_group_by_w_ctx() {
    ecs_world_t *world = ecs_init();

    ECS_TAG(world, TagA);
    ECS_TAG(world, TagB);
    ECS_TAG(world, TagC);
    ECS_TAG(world, TagX);

    ecs_query_t *q = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter.terms = {{TagX}},
        .group_by = group_by_first_id,
        .group_by_ctx = &ctx_value,
        .group_by_ctx_free = ctx_value_free
    });

    ecs_entity_t e1 = ecs_new(world, TagX);
    ecs_entity_t e2 = ecs_new(world, TagX);
    ecs_entity_t e3 = ecs_new(world, TagX);

    ecs_add_id(world, e1, TagC);
    ecs_add_id(world, e2, TagB);
    ecs_add_id(world, e3, TagA);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_bool(ecs_query_next(&it), true);
    test_int(it.count, 1);
    test_int(it.entities[0], e3);

    test_bool(ecs_query_next(&it), true);
    test_int(it.count, 1);
    test_int(it.entities[0], e2);

    test_bool(ecs_query_next(&it), true);
    test_int(it.count, 1);
    test_int(it.entities[0], e1);    

    test_bool(ecs_query_next(&it), false);

    ecs_query_fini(q);

    test_int(ctx_value_free_invoked, 1);

    ecs_fini(world);
}

void Query_group_by_w_sort_reverse_group_creation() {
    ecs_world_t *world = ecs_init();

    ecs_entity_t TagA = ecs_new_id(world);
    ecs_entity_t TagB = ecs_new_id(world);
    ecs_entity_t TagC = ecs_new_id(world);

    ecs_entity_t TagX = ecs_new_id(world);

    ecs_query_t *q = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter.terms = {{TagX}},
        .order_by = order_by_entity,
        .group_by = group_by_first_id
    });

    ecs_entity_t e1 = ecs_new_w_id(world, TagX);
    ecs_entity_t e2 = ecs_new_w_id(world, TagX);
    ecs_entity_t e3 = ecs_new_w_id(world, TagX);

    ecs_add_id(world, e1, TagC);
    ecs_add_id(world, e2, TagB);
    ecs_add_id(world, e3, TagA);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_bool(ecs_query_next(&it), true);
    test_int(it.count, 1);
    test_int(it.entities[0], e3);

    test_bool(ecs_query_next(&it), true);
    test_int(it.count, 1);
    test_int(it.entities[0], e2);

    test_bool(ecs_query_next(&it), true);
    test_int(it.count, 1);
    test_int(it.entities[0], e1);    

    test_bool(ecs_query_next(&it), false);

    ecs_fini(world);
}

void Query_iter_valid() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    
    ecs_new(world, Position);

    ecs_query_t *q = ecs_query_new(world, "Position");
    test_assert(q != NULL);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_bool(it.is_valid, false);

    test_bool(ecs_query_next(&it), true);
    test_bool(it.is_valid, true);

    test_bool(ecs_query_next(&it), false);
    test_bool(it.is_valid, false);

    ecs_fini(world);
}

void Query_query_optional_tag() {
    ecs_world_t *world = ecs_init();

    ECS_TAG(world, TagA);
    ECS_TAG(world, TagB);
    
    ecs_entity_t e1 = ecs_new(world, TagA);
    ecs_entity_t e2 = ecs_new(world, TagA);
    ecs_add_id(world, e2, TagB);

    ecs_query_t *q = ecs_query_new(world, "TagA, ?TagB");
    test_assert(q != NULL);

    ecs_iter_t it = ecs_query_iter(world, q);
    int32_t count = 0;
    
    while (ecs_query_next(&it)) {    
        test_assert(ecs_term_id(&it, 1) == TagA);
        test_assert(ecs_term_id(&it, 2) == TagB);
        test_int(it.count, 1);

        if (it.entities[0] == e1) {
            test_bool(ecs_term_is_set(&it, 1), true);
            test_bool(ecs_term_is_set(&it, 2), false); 
        } else if (it.entities[0] == e2) {
            test_bool(ecs_term_is_set(&it, 1), true);
            test_bool(ecs_term_is_set(&it, 2), true); 
        }

        count ++;
    }

    test_int(count, 2);

    ecs_fini(world);
}

void Query_query_optional_shared_tag() {
    ecs_world_t *world = ecs_init();

    ECS_TAG(world, TagA);
    ECS_TAG(world, TagB);
    
    ecs_entity_t e1 = ecs_new(world, TagA);
    ecs_entity_t e2 = ecs_new(world, TagA);
    ecs_add_id(world, e2, TagB);
    
    ecs_entity_t e3 = ecs_new(world, TagA);
    ecs_add_pair(world, e3, EcsIsA, e2);

    ecs_query_t *q = ecs_query_new(world, "TagA, ?TagB(self|super)");
    test_assert(q != NULL);

    ecs_iter_t it = ecs_query_iter(world, q);
    int32_t count = 0;
    
    while (ecs_query_next(&it)) {    
        test_assert(ecs_term_id(&it, 1) == TagA);
        test_assert(ecs_term_id(&it, 2) == TagB);
        test_int(it.count, 1);

        if (it.entities[0] == e1) {
            test_bool(ecs_term_is_set(&it, 1), true);
            test_bool(ecs_term_is_set(&it, 2), false); 
        } else if (it.entities[0] == e2) {
            test_bool(ecs_term_is_set(&it, 1), true);
            test_bool(ecs_term_is_set(&it, 2), true); 
        } else if (it.entities[0] == e3) {
            test_bool(ecs_term_is_set(&it, 1), true);
            test_bool(ecs_term_is_set(&it, 2), true);
        }

        count ++;
    }

    test_int(count, 3);

    ecs_fini(world);
}

void Query_query_iter_10_tags() {
    ecs_world_t *world = ecs_init();

    ECS_TAG(world, TagA);
    ECS_TAG(world, TagB);
    ECS_TAG(world, TagC);
    ECS_TAG(world, TagD);
    ECS_TAG(world, TagE);
    ECS_TAG(world, TagF);
    ECS_TAG(world, TagG);
    ECS_TAG(world, TagH);
    ECS_TAG(world, TagI);
    ECS_TAG(world, TagJ);
    ECS_TAG(world, TagK);

    ecs_entity_t e_1 = ecs_new(world, TagA);
    ecs_add_id(world, e_1, TagB);
    ecs_add_id(world, e_1, TagC);
    ecs_add_id(world, e_1, TagD);
    ecs_add_id(world, e_1, TagE);
    ecs_add_id(world, e_1, TagF);
    ecs_add_id(world, e_1, TagG);
    ecs_add_id(world, e_1, TagH);
    ecs_add_id(world, e_1, TagI);
    ecs_add_id(world, e_1, TagJ);
    
    ecs_entity_t e_2 = ecs_new(world, TagA);
    ecs_add_id(world, e_2, TagB);
    ecs_add_id(world, e_2, TagC);
    ecs_add_id(world, e_2, TagD);
    ecs_add_id(world, e_2, TagE);
    ecs_add_id(world, e_2, TagF);
    ecs_add_id(world, e_2, TagG);
    ecs_add_id(world, e_2, TagH);
    ecs_add_id(world, e_2, TagI);
    ecs_add_id(world, e_2, TagJ);
    ecs_add_id(world, e_2, TagK); /* 2nd match in different table */

    ecs_query_t *q = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter.terms = {
            {TagA}, {TagB}, {TagC}, {TagD}, {TagE}, {TagF}, {TagG}, {TagH}, 
            {TagI}, {TagJ}
        }
    });

    ecs_iter_t it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_int(it.entities[0], e_1);
    test_int(ecs_term_id(&it, 1), TagA);
    test_int(ecs_term_id(&it, 2), TagB);
    test_int(ecs_term_id(&it, 3), TagC);
    test_int(ecs_term_id(&it, 4), TagD);
    test_int(ecs_term_id(&it, 5), TagE);
    test_int(ecs_term_id(&it, 6), TagF);
    test_int(ecs_term_id(&it, 7), TagG);
    test_int(ecs_term_id(&it, 8), TagH);
    test_int(ecs_term_id(&it, 9), TagI);
    test_int(ecs_term_id(&it, 10), TagJ);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_int(it.entities[0], e_2);
    test_int(ecs_term_id(&it, 1), TagA);
    test_int(ecs_term_id(&it, 2), TagB);
    test_int(ecs_term_id(&it, 3), TagC);
    test_int(ecs_term_id(&it, 4), TagD);
    test_int(ecs_term_id(&it, 5), TagE);
    test_int(ecs_term_id(&it, 6), TagF);
    test_int(ecs_term_id(&it, 7), TagG);
    test_int(ecs_term_id(&it, 8), TagH);
    test_int(ecs_term_id(&it, 9), TagI);
    test_int(ecs_term_id(&it, 10), TagJ);

    test_assert(!ecs_query_next(&it));

    ecs_query_fini(q);

    ecs_fini(world);
}

void Query_query_iter_20_tags() {
    ecs_world_t *world = ecs_init();

    ECS_TAG(world, TagA);
    ECS_TAG(world, TagB);
    ECS_TAG(world, TagC);
    ECS_TAG(world, TagD);
    ECS_TAG(world, TagE);
    ECS_TAG(world, TagF);
    ECS_TAG(world, TagG);
    ECS_TAG(world, TagH);
    ECS_TAG(world, TagI);
    ECS_TAG(world, TagJ);
    ECS_TAG(world, TagK);
    ECS_TAG(world, TagL);
    ECS_TAG(world, TagM);
    ECS_TAG(world, TagN);
    ECS_TAG(world, TagO);
    ECS_TAG(world, TagP);
    ECS_TAG(world, TagQ);
    ECS_TAG(world, TagR);
    ECS_TAG(world, TagS);
    ECS_TAG(world, TagT);
    ECS_TAG(world, TagU);

    ecs_entity_t e_1 = ecs_new(world, TagA);
    ecs_add_id(world, e_1, TagB);
    ecs_add_id(world, e_1, TagC);
    ecs_add_id(world, e_1, TagD);
    ecs_add_id(world, e_1, TagE);
    ecs_add_id(world, e_1, TagF);
    ecs_add_id(world, e_1, TagG);
    ecs_add_id(world, e_1, TagH);
    ecs_add_id(world, e_1, TagI);
    ecs_add_id(world, e_1, TagJ);
    ecs_add_id(world, e_1, TagK);
    ecs_add_id(world, e_1, TagL);
    ecs_add_id(world, e_1, TagM);
    ecs_add_id(world, e_1, TagN);
    ecs_add_id(world, e_1, TagO);
    ecs_add_id(world, e_1, TagP);
    ecs_add_id(world, e_1, TagQ);
    ecs_add_id(world, e_1, TagR);
    ecs_add_id(world, e_1, TagS);
    ecs_add_id(world, e_1, TagT);
    
    ecs_entity_t e_2 = ecs_new(world, TagA);
    ecs_add_id(world, e_2, TagB);
    ecs_add_id(world, e_2, TagC);
    ecs_add_id(world, e_2, TagD);
    ecs_add_id(world, e_2, TagE);
    ecs_add_id(world, e_2, TagF);
    ecs_add_id(world, e_2, TagG);
    ecs_add_id(world, e_2, TagH);
    ecs_add_id(world, e_2, TagI);
    ecs_add_id(world, e_2, TagJ);
    ecs_add_id(world, e_2, TagK);
    ecs_add_id(world, e_2, TagL);
    ecs_add_id(world, e_2, TagM);
    ecs_add_id(world, e_2, TagN);
    ecs_add_id(world, e_2, TagO);
    ecs_add_id(world, e_2, TagP);
    ecs_add_id(world, e_2, TagQ);
    ecs_add_id(world, e_2, TagR);
    ecs_add_id(world, e_2, TagS);
    ecs_add_id(world, e_2, TagT);
    ecs_add_id(world, e_2, TagU); /* 2nd match in different table */

    ecs_query_t *q = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter = {
            .terms_buffer = (ecs_term_t[]){
                {TagA}, {TagB}, {TagC}, {TagD}, {TagE}, {TagF}, {TagG}, {TagH}, 
                {TagI}, {TagJ}, {TagK}, {TagL}, {TagM}, {TagN}, {TagO}, {TagP},
                {TagQ}, {TagR}, {TagS}, {TagT}
            },
            .terms_buffer_count = 20
        },
    });

    ecs_iter_t it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_int(it.entities[0], e_1);
    test_int(ecs_term_id(&it, 1), TagA);
    test_int(ecs_term_id(&it, 2), TagB);
    test_int(ecs_term_id(&it, 3), TagC);
    test_int(ecs_term_id(&it, 4), TagD);
    test_int(ecs_term_id(&it, 5), TagE);
    test_int(ecs_term_id(&it, 6), TagF);
    test_int(ecs_term_id(&it, 7), TagG);
    test_int(ecs_term_id(&it, 8), TagH);
    test_int(ecs_term_id(&it, 9), TagI);
    test_int(ecs_term_id(&it, 10), TagJ);
    test_int(ecs_term_id(&it, 11), TagK);
    test_int(ecs_term_id(&it, 12), TagL);
    test_int(ecs_term_id(&it, 13), TagM);
    test_int(ecs_term_id(&it, 14), TagN);
    test_int(ecs_term_id(&it, 15), TagO);
    test_int(ecs_term_id(&it, 16), TagP);
    test_int(ecs_term_id(&it, 17), TagQ);
    test_int(ecs_term_id(&it, 18), TagR);
    test_int(ecs_term_id(&it, 19), TagS);
    test_int(ecs_term_id(&it, 20), TagT);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_int(it.entities[0], e_2);
    test_int(ecs_term_id(&it, 1), TagA);
    test_int(ecs_term_id(&it, 2), TagB);
    test_int(ecs_term_id(&it, 3), TagC);
    test_int(ecs_term_id(&it, 4), TagD);
    test_int(ecs_term_id(&it, 5), TagE);
    test_int(ecs_term_id(&it, 6), TagF);
    test_int(ecs_term_id(&it, 7), TagG);
    test_int(ecs_term_id(&it, 8), TagH);
    test_int(ecs_term_id(&it, 9), TagI);
    test_int(ecs_term_id(&it, 10), TagJ);
    test_int(ecs_term_id(&it, 11), TagK);
    test_int(ecs_term_id(&it, 12), TagL);
    test_int(ecs_term_id(&it, 13), TagM);
    test_int(ecs_term_id(&it, 14), TagN);
    test_int(ecs_term_id(&it, 15), TagO);
    test_int(ecs_term_id(&it, 16), TagP);
    test_int(ecs_term_id(&it, 17), TagQ);
    test_int(ecs_term_id(&it, 18), TagR);
    test_int(ecs_term_id(&it, 19), TagS);
    test_int(ecs_term_id(&it, 20), TagT);

    test_assert(!ecs_query_next(&it));

    ecs_fini(world);
}

typedef struct {
    float v;
} CompA, CompB, CompC, CompD, CompE, CompF, CompG, CompH, CompI, CompJ, CompK,
  CompL, CompM, CompN, CompO, CompP, CompQ, CompR, CompS, CompT, CompU;

void Query_query_iter_10_components() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, CompA);
    ECS_COMPONENT(world, CompB);
    ECS_COMPONENT(world, CompC);
    ECS_COMPONENT(world, CompD);
    ECS_COMPONENT(world, CompE);
    ECS_COMPONENT(world, CompF);
    ECS_COMPONENT(world, CompG);
    ECS_COMPONENT(world, CompH);
    ECS_COMPONENT(world, CompI);
    ECS_COMPONENT(world, CompJ);
    ECS_COMPONENT(world, CompK);

    ecs_entity_t e_1 = ecs_set(world, 0, CompA, {10});
    ecs_set(world, e_1, CompB, {10});
    ecs_set(world, e_1, CompC, {10});
    ecs_set(world, e_1, CompD, {10});
    ecs_set(world, e_1, CompE, {10});
    ecs_set(world, e_1, CompF, {10});
    ecs_set(world, e_1, CompG, {10});
    ecs_set(world, e_1, CompH, {10});
    ecs_set(world, e_1, CompI, {10});
    ecs_set(world, e_1, CompJ, {10});
    
    ecs_entity_t e_2 = ecs_set(world, 0, CompA, {10});
    ecs_set(world, e_2, CompB, {10});
    ecs_set(world, e_2, CompC, {10});
    ecs_set(world, e_2, CompD, {10});
    ecs_set(world, e_2, CompE, {10});
    ecs_set(world, e_2, CompF, {10});
    ecs_set(world, e_2, CompG, {10});
    ecs_set(world, e_2, CompH, {10});
    ecs_set(world, e_2, CompI, {10});
    ecs_set(world, e_2, CompJ, {10});
    ecs_set(world, e_2, CompK, {10});

    ecs_query_t *q = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter.terms = {
            {ecs_id(CompA)}, {ecs_id(CompB)}, {ecs_id(CompC)}, {ecs_id(CompD)}, 
            {ecs_id(CompE)}, {ecs_id(CompF)}, {ecs_id(CompG)}, {ecs_id(CompH)}, 
            {ecs_id(CompI)}, {ecs_id(CompJ)}
        }
    });

    ecs_iter_t it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_int(it.entities[0], e_1);
    test_int(ecs_term_id(&it, 1), ecs_id(CompA));
    test_int(ecs_term_id(&it, 2), ecs_id(CompB));
    test_int(ecs_term_id(&it, 3), ecs_id(CompC));
    test_int(ecs_term_id(&it, 4), ecs_id(CompD));
    test_int(ecs_term_id(&it, 5), ecs_id(CompE));
    test_int(ecs_term_id(&it, 6), ecs_id(CompF));
    test_int(ecs_term_id(&it, 7), ecs_id(CompG));
    test_int(ecs_term_id(&it, 8), ecs_id(CompH));
    test_int(ecs_term_id(&it, 9), ecs_id(CompI));
    test_int(ecs_term_id(&it, 10), ecs_id(CompJ));

    int i;
    for (i = 0; i < 10; i ++) {
        CompA *ptr = ecs_term_w_size(&it, 0, i + 1);
        test_assert(ptr != NULL);
        test_int(ptr[0].v, 10);
    }

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_int(it.entities[0], e_2);
    test_int(ecs_term_id(&it, 1), ecs_id(CompA));
    test_int(ecs_term_id(&it, 2), ecs_id(CompB));
    test_int(ecs_term_id(&it, 3), ecs_id(CompC));
    test_int(ecs_term_id(&it, 4), ecs_id(CompD));
    test_int(ecs_term_id(&it, 5), ecs_id(CompE));
    test_int(ecs_term_id(&it, 6), ecs_id(CompF));
    test_int(ecs_term_id(&it, 7), ecs_id(CompG));
    test_int(ecs_term_id(&it, 8), ecs_id(CompH));
    test_int(ecs_term_id(&it, 9), ecs_id(CompI));
    test_int(ecs_term_id(&it, 10), ecs_id(CompJ));

    for (i = 0; i < 10; i ++) {
        CompA *ptr = ecs_term_w_size(&it, 0, i + 1);
        test_assert(ptr != NULL);
        test_int(ptr[0].v, 10);
    }

    test_assert(!ecs_query_next(&it));

    ecs_fini(world);
}

void Query_query_iter_20_components() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, CompA);
    ECS_COMPONENT(world, CompB);
    ECS_COMPONENT(world, CompC);
    ECS_COMPONENT(world, CompD);
    ECS_COMPONENT(world, CompE);
    ECS_COMPONENT(world, CompF);
    ECS_COMPONENT(world, CompG);
    ECS_COMPONENT(world, CompH);
    ECS_COMPONENT(world, CompI);
    ECS_COMPONENT(world, CompJ);
    ECS_COMPONENT(world, CompK);
    ECS_COMPONENT(world, CompL);
    ECS_COMPONENT(world, CompM);
    ECS_COMPONENT(world, CompN);
    ECS_COMPONENT(world, CompO);
    ECS_COMPONENT(world, CompP);
    ECS_COMPONENT(world, CompQ);
    ECS_COMPONENT(world, CompR);
    ECS_COMPONENT(world, CompS);
    ECS_COMPONENT(world, CompT);
    ECS_COMPONENT(world, CompU);

    ecs_entity_t e_1 = ecs_set(world, 0, CompA, {10});
    ecs_set(world, e_1, CompB, {10});
    ecs_set(world, e_1, CompC, {10});
    ecs_set(world, e_1, CompD, {10});
    ecs_set(world, e_1, CompE, {10});
    ecs_set(world, e_1, CompF, {10});
    ecs_set(world, e_1, CompG, {10});
    ecs_set(world, e_1, CompH, {10});
    ecs_set(world, e_1, CompI, {10});
    ecs_set(world, e_1, CompJ, {10});
    ecs_set(world, e_1, CompK, {10});
    ecs_set(world, e_1, CompL, {10});
    ecs_set(world, e_1, CompM, {10});
    ecs_set(world, e_1, CompN, {10});
    ecs_set(world, e_1, CompO, {10});
    ecs_set(world, e_1, CompP, {10});
    ecs_set(world, e_1, CompQ, {10});
    ecs_set(world, e_1, CompR, {10});
    ecs_set(world, e_1, CompS, {10});
    ecs_set(world, e_1, CompT, {10});

    ecs_entity_t e_2 = ecs_set(world, 0, CompA, {10});
    ecs_set(world, e_2, CompB, {10});
    ecs_set(world, e_2, CompC, {10});
    ecs_set(world, e_2, CompD, {10});
    ecs_set(world, e_2, CompE, {10});
    ecs_set(world, e_2, CompF, {10});
    ecs_set(world, e_2, CompG, {10});
    ecs_set(world, e_2, CompH, {10});
    ecs_set(world, e_2, CompI, {10});
    ecs_set(world, e_2, CompJ, {10});
    ecs_set(world, e_2, CompK, {10});
    ecs_set(world, e_2, CompL, {10});
    ecs_set(world, e_2, CompM, {10});
    ecs_set(world, e_2, CompN, {10});
    ecs_set(world, e_2, CompO, {10});
    ecs_set(world, e_2, CompP, {10});
    ecs_set(world, e_2, CompQ, {10});
    ecs_set(world, e_2, CompR, {10});
    ecs_set(world, e_2, CompS, {10});
    ecs_set(world, e_2, CompT, {10});
    ecs_set(world, e_2, CompU, {10});

    ecs_query_t *q = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter = {
            .terms_buffer = (ecs_term_t[]){
                {ecs_id(CompA)}, {ecs_id(CompB)}, {ecs_id(CompC)}, {ecs_id(CompD)}, 
                {ecs_id(CompE)}, {ecs_id(CompF)}, {ecs_id(CompG)}, {ecs_id(CompH)}, 
                {ecs_id(CompI)}, {ecs_id(CompJ)}, {ecs_id(CompK)}, {ecs_id(CompL)},
                {ecs_id(CompM)}, {ecs_id(CompN)}, {ecs_id(CompO)}, {ecs_id(CompP)},
                {ecs_id(CompQ)}, {ecs_id(CompR)}, {ecs_id(CompS)}, {ecs_id(CompT)}
            },
            .terms_buffer_count = 20
        }
    });

    ecs_iter_t it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_int(it.entities[0], e_1);
    test_int(ecs_term_id(&it, 1), ecs_id(CompA));
    test_int(ecs_term_id(&it, 2), ecs_id(CompB));
    test_int(ecs_term_id(&it, 3), ecs_id(CompC));
    test_int(ecs_term_id(&it, 4), ecs_id(CompD));
    test_int(ecs_term_id(&it, 5), ecs_id(CompE));
    test_int(ecs_term_id(&it, 6), ecs_id(CompF));
    test_int(ecs_term_id(&it, 7), ecs_id(CompG));
    test_int(ecs_term_id(&it, 8), ecs_id(CompH));
    test_int(ecs_term_id(&it, 9), ecs_id(CompI));
    test_int(ecs_term_id(&it, 10), ecs_id(CompJ));
    test_int(ecs_term_id(&it, 11), ecs_id(CompK));
    test_int(ecs_term_id(&it, 12), ecs_id(CompL));
    test_int(ecs_term_id(&it, 13), ecs_id(CompM));
    test_int(ecs_term_id(&it, 14), ecs_id(CompN));
    test_int(ecs_term_id(&it, 15), ecs_id(CompO));
    test_int(ecs_term_id(&it, 16), ecs_id(CompP));
    test_int(ecs_term_id(&it, 17), ecs_id(CompQ));
    test_int(ecs_term_id(&it, 18), ecs_id(CompR));
    test_int(ecs_term_id(&it, 19), ecs_id(CompS));
    test_int(ecs_term_id(&it, 20), ecs_id(CompT));

    int i;
    for (i = 0; i < 20; i ++) {
        CompA *ptr = ecs_term_w_size(&it, 0, i + 1);
        test_assert(ptr != NULL);
        test_int(ptr[0].v, 10);
    }

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_int(it.entities[0], e_2);
    test_int(ecs_term_id(&it, 1), ecs_id(CompA));
    test_int(ecs_term_id(&it, 2), ecs_id(CompB));
    test_int(ecs_term_id(&it, 3), ecs_id(CompC));
    test_int(ecs_term_id(&it, 4), ecs_id(CompD));
    test_int(ecs_term_id(&it, 5), ecs_id(CompE));
    test_int(ecs_term_id(&it, 6), ecs_id(CompF));
    test_int(ecs_term_id(&it, 7), ecs_id(CompG));
    test_int(ecs_term_id(&it, 8), ecs_id(CompH));
    test_int(ecs_term_id(&it, 9), ecs_id(CompI));
    test_int(ecs_term_id(&it, 10), ecs_id(CompJ));
    test_int(ecs_term_id(&it, 11), ecs_id(CompK));
    test_int(ecs_term_id(&it, 12), ecs_id(CompL));
    test_int(ecs_term_id(&it, 13), ecs_id(CompM));
    test_int(ecs_term_id(&it, 14), ecs_id(CompN));
    test_int(ecs_term_id(&it, 15), ecs_id(CompO));
    test_int(ecs_term_id(&it, 16), ecs_id(CompP));
    test_int(ecs_term_id(&it, 17), ecs_id(CompQ));
    test_int(ecs_term_id(&it, 18), ecs_id(CompR));
    test_int(ecs_term_id(&it, 19), ecs_id(CompS));
    test_int(ecs_term_id(&it, 20), ecs_id(CompT));

    for (i = 0; i < 20; i ++) {
        CompA *ptr = ecs_term_w_size(&it, 0, i + 1);
        test_assert(ptr != NULL);
        test_int(ptr[0].v, 10);
    }

    test_assert(!ecs_query_next(&it));

    ecs_fini(world);
}

void Query_iter_type_set() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    
    ecs_entity_t e = ecs_new(world, Position);

    ecs_query_t *q = ecs_query_new(world, "Position");
    test_assert(q != NULL);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_int(it.entities[0], e);
    test_assert(it.type != NULL);
    test_int(ecs_vector_count(it.type), 1);
    test_int(ecs_vector_first(it.type, ecs_id_t)[0], ecs_id(Position));

    test_assert(!ecs_query_next(&it));

    ecs_fini(world);
}

void Query_only_optional_from_entity() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    ECS_ENTITY(world, Ent, Position);

    ecs_set(world, Ent, Position, {10, 20});

    ecs_query_t *q = ecs_query_new(world, "?Position(Ent)");
    test_assert(q != NULL);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_next(&it));
    test_int(it.count, 0);

    test_bool(ecs_term_is_set(&it, 1), true);
    
    Position *ptr = ecs_term(&it, Position, 1);
    test_assert(ptr != NULL);
    test_int(ptr->x, 10);
    test_int(ptr->y, 20);

    test_assert(!ecs_query_next(&it));

    ecs_fini(world);
}

void Query_only_optional_from_entity_no_match() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    ECS_ENTITY(world, Ent, 0);

    ecs_query_t *q = ecs_query_new(world, "?Position(Ent)");
    test_assert(q != NULL);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_next(&it));
    test_int(it.count, 0);

    test_bool(ecs_term_is_set(&it, 1), false);

    test_assert(!ecs_query_next(&it));

    ecs_fini(world);
}

void Query_filter_term() {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);

    ecs_query_t *q = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter.terms = {{ .id = ecs_id(Position), .inout = EcsInOutFilter }}
    });

    ecs_entity_t e = ecs_set(world, 0, Position, {10, 20});

    ecs_iter_t it = ecs_query_iter(world, q);

    test_bool(ecs_query_next(&it), true);
    test_assert(it.ids != NULL);
    test_assert(it.ids[0] == ecs_id(Position));

    test_int(it.count, 1);
    test_assert(it.entities != NULL);
    test_assert(it.entities[0] == e);

    test_assert(it.ptrs == NULL);
    test_assert(it.sizes == NULL);
    test_assert(it.columns != NULL);

    test_bool(ecs_query_next(&it), false);

    ecs_fini(world);
}

void Query_2_terms_1_filter() {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_query_t *q = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter.terms = {
            { .id = ecs_id(Position), .inout = EcsInOutFilter },
            { .id = ecs_id(Velocity) }
        }
    });

    ecs_entity_t e = ecs_set(world, 0, Position, {10, 20});
    ecs_set(world, e, Velocity, {1, 1});

    ecs_iter_t it = ecs_query_iter(world, q);

    test_bool(ecs_query_next(&it), true);
    test_assert(it.ids != NULL);
    test_assert(it.ids[0] == ecs_id(Position));
    test_assert(it.ids[1] == ecs_id(Velocity));

    test_int(it.count, 1);
    test_assert(it.entities != NULL);
    test_assert(it.entities[0] == e);

    test_assert(it.ptrs != NULL);
    test_assert(it.sizes != NULL);
    test_assert(it.columns != NULL);

    test_assert(it.ptrs[0] == NULL);
    test_assert(it.ptrs[1] != NULL);

    test_bool(ecs_query_next(&it), false);

    ecs_fini(world);
}

void Query_3_terms_2_filter() {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);

    ecs_query_t *q = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter.terms = {
            { .id = ecs_id(Position), .inout = EcsInOutFilter },
            { .id = ecs_id(Velocity), .inout = EcsInOutFilter },
            { .id = ecs_id(Mass) }
        }
    });

    ecs_entity_t e = ecs_set(world, 0, Position, {10, 20});
    ecs_set(world, e, Velocity, {1, 1});
    ecs_set(world, e, Mass, {1});

    ecs_iter_t it = ecs_query_iter(world, q);

    test_bool(ecs_query_next(&it), true);
    test_assert(it.ids != NULL);
    test_assert(it.ids[0] == ecs_id(Position));
    test_assert(it.ids[1] == ecs_id(Velocity));
    test_assert(it.ids[2] == ecs_id(Mass));

    test_int(it.count, 1);
    test_assert(it.entities != NULL);
    test_assert(it.entities[0] == e);

    test_assert(it.ptrs != NULL);
    test_assert(it.sizes != NULL);
    test_assert(it.columns != NULL);

    test_assert(it.ptrs[0] == NULL);
    test_assert(it.ptrs[1] == NULL);
    test_assert(it.ptrs[2] != NULL);

    test_bool(ecs_query_next(&it), false);

    ecs_fini(world);
}

void Query_no_instancing_w_singleton() {
    ecs_world_t *world = ecs_init();
    
    ECS_TAG(world, Tag);

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_singleton_set(world, Velocity, {1, 2});

    ecs_entity_t e1 = ecs_set(world, 0, Position, {10, 20});
    ecs_entity_t e2 = ecs_set(world, 0, Position, {20, 30});
    ecs_entity_t e3 = ecs_set(world, 0, Position, {30, 40});

    ecs_entity_t e4 = ecs_set(world, 0, Position, {40, 50});
    ecs_entity_t e5 = ecs_set(world, 0, Position, {50, 60});

    ecs_add(world, e4, Tag);
    ecs_add(world, e5, Tag);

    ecs_query_t *q = ecs_query_new(world, "Position, $Velocity");
    test_assert(q != NULL);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_next(&it));
    {
        Position *p = ecs_term(&it, Position, 1);
        Velocity *v = ecs_term(&it, Velocity, 2);
        test_int(it.count, 1);
        test_int(it.entities[0], e1);
        test_int(p->x, 10);
        test_int(p->y, 20);
        test_int(v->x, 1);
        test_int(v->y, 2);
    }

    test_assert(ecs_query_next(&it));
    {
        Position *p = ecs_term(&it, Position, 1);
        Velocity *v = ecs_term(&it, Velocity, 2);
        test_int(it.count, 1);
        test_int(it.entities[0], e2);
        test_int(p->x, 20);
        test_int(p->y, 30);
        test_int(v->x, 1);
        test_int(v->y, 2);
    }

    test_assert(ecs_query_next(&it));
    {
        Position *p = ecs_term(&it, Position, 1);
        Velocity *v = ecs_term(&it, Velocity, 2);
        test_int(it.count, 1);
        test_int(it.entities[0], e3);
        test_int(p->x, 30);
        test_int(p->y, 40);
        test_int(v->x, 1);
        test_int(v->y, 2);
    }

    test_assert(ecs_query_next(&it));
    {
        Position *p = ecs_term(&it, Position, 1);
        Velocity *v = ecs_term(&it, Velocity, 2);
        test_int(it.count, 1);
        test_int(it.entities[0], e4);
        test_int(p->x, 40);
        test_int(p->y, 50);
        test_int(v->x, 1);
        test_int(v->y, 2);
    }

    test_assert(ecs_query_next(&it));
    {
        Position *p = ecs_term(&it, Position, 1);
        Velocity *v = ecs_term(&it, Velocity, 2);
        test_int(it.count, 1);
        test_int(it.entities[0], e5);
        test_int(p->x, 50);
        test_int(p->y, 60);
        test_int(v->x, 1);
        test_int(v->y, 2);
    }

    test_assert(!ecs_query_next(&it));

    ecs_fini(world);
}

void Query_no_instancing_w_shared() {
    ecs_world_t *world = ecs_init();
    
    ECS_TAG(world, Tag);

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_entity_t base = ecs_set(world, 0, Velocity, {1, 2});

    ecs_entity_t e1 = ecs_set(world, 0, Position, {10, 20});
    ecs_entity_t e2 = ecs_set(world, 0, Position, {20, 30});
    ecs_entity_t e3 = ecs_set(world, 0, Position, {30, 40});

    ecs_entity_t e4 = ecs_set(world, 0, Position, {40, 50});
    ecs_entity_t e5 = ecs_set(world, 0, Position, {50, 60});
    ecs_add(world, e4, Tag);
    ecs_add(world, e5, Tag);

    ecs_add_pair(world, e1, EcsIsA, base);
    ecs_add_pair(world, e2, EcsIsA, base);
    ecs_add_pair(world, e3, EcsIsA, base);
    ecs_add_pair(world, e4, EcsIsA, base);
    ecs_add_pair(world, e5, EcsIsA, base);

    ecs_entity_t e6 = ecs_set(world, 0, Position, {60, 70});
    ecs_entity_t e7 = ecs_set(world, 0, Position, {70, 80});
    ecs_set(world, e6, Velocity, {2, 3});
    ecs_set(world, e7, Velocity, {4, 5});

    ecs_query_t *q = ecs_query_new(world, "Position, Velocity");
    test_assert(q != NULL);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_next(&it));
    {
        Position *p = ecs_term(&it, Position, 1);
        Velocity *v = ecs_term(&it, Velocity, 2);
        test_int(it.count, 1);
        test_int(it.entities[0], e1);
        test_int(p->x, 10);
        test_int(p->y, 20);
        test_int(v->x, 1);
        test_int(v->y, 2);
    }

    test_assert(ecs_query_next(&it));
    {
        Position *p = ecs_term(&it, Position, 1);
        Velocity *v = ecs_term(&it, Velocity, 2);
        test_int(it.count, 1);
        test_int(it.entities[0], e2);
        test_int(p->x, 20);
        test_int(p->y, 30);
        test_int(v->x, 1);
        test_int(v->y, 2);
    }

    test_assert(ecs_query_next(&it));
    {
        Position *p = ecs_term(&it, Position, 1);
        Velocity *v = ecs_term(&it, Velocity, 2);
        test_int(it.count, 1);
        test_int(it.entities[0], e3);
        test_int(p->x, 30);
        test_int(p->y, 40);
        test_int(v->x, 1);
        test_int(v->y, 2);
    }

    test_assert(ecs_query_next(&it));
    {
        Position *p = ecs_term(&it, Position, 1);
        Velocity *v = ecs_term(&it, Velocity, 2);
        test_int(it.count, 1);
        test_int(it.entities[0], e4);
        test_int(p->x, 40);
        test_int(p->y, 50);
        test_int(v->x, 1);
        test_int(v->y, 2);
    }

    test_assert(ecs_query_next(&it));
    {
        Position *p = ecs_term(&it, Position, 1);
        Velocity *v = ecs_term(&it, Velocity, 2);
        test_int(it.count, 1);
        test_int(it.entities[0], e5);
        test_int(p->x, 50);
        test_int(p->y, 60);
        test_int(v->x, 1);
        test_int(v->y, 2);
    }

    test_assert(ecs_query_next(&it));
    {
        Position *p = ecs_term(&it, Position, 1);
        Velocity *v = ecs_term(&it, Velocity, 2);
        test_int(it.count, 2);
        test_int(it.entities[0], e6);
        test_int(p[0].x, 60);
        test_int(p[0].y, 70);
        test_int(v[0].x, 2);
        test_int(v[0].y, 3);

        test_int(it.entities[1], e7);
        test_int(p[1].x, 70);
        test_int(p[1].y, 80);
        test_int(v[1].x, 4);
        test_int(v[1].y, 5);
    }

    test_assert(!ecs_query_next(&it));

    ecs_fini(world);
}

void Query_query_iter_frame_offset() {
    ecs_world_t *world = ecs_mini();

    ECS_TAG(world, TagA);
    ECS_TAG(world, TagB);
    ECS_TAG(world, TagC);

    ecs_query_t *q = ecs_query_init(world, &(ecs_query_desc_t) {
        .filter.terms = {
            { .id = TagA, }
        },
    });

    ecs_entity_t e1 = ecs_new(world, TagA);
    ecs_entity_t e2 = ecs_new(world, TagA);
    ecs_entity_t e3 = ecs_new(world, TagA);
    ecs_entity_t e4 = ecs_new(world, TagA);
    ecs_entity_t e5 = ecs_new(world, TagA);

    ecs_add(world, e3, TagB);
    ecs_add(world, e4, TagB);
    ecs_add(world, e5, TagC);

    ecs_iter_t it = ecs_query_iter(world, q);

    test_bool(ecs_query_next(&it), true);
    test_int(it.count, 2);
    test_int(it.frame_offset, 0);
    test_assert(it.entities != NULL);
    test_assert(it.entities[0] == e1);
    test_assert(it.entities[1] == e2);
    test_assert(it.ids[0] == TagA);

    test_bool(ecs_query_next(&it), true);
    test_int(it.count, 2);
    test_int(it.frame_offset, 2);
    test_assert(it.entities != NULL);
    test_assert(it.entities[0] == e3);
    test_assert(it.entities[1] == e4);
    test_assert(it.ids[0] == TagA);

    test_bool(ecs_query_next(&it), true);
    test_int(it.count, 1);
    test_int(it.frame_offset, 4);
    test_assert(it.entities != NULL);
    test_assert(it.entities[0] == e5);
    test_assert(it.ids[0] == TagA);

    test_bool(ecs_query_next(&it), false);

    ecs_fini(world);
}
