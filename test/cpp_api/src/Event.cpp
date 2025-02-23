#include <cpp_api.h>

struct Evt { };

struct IdA { };
struct IdB { };

void Event_evt_1_id_entity() {
    flecs::world ecs;

    auto evt = ecs.entity();
    auto id = ecs.entity();
    auto e1 = ecs.entity().add(id);

    int32_t count = 0;

    ecs.trigger()
        .event(evt)
        .id(id)
        .each([&](flecs::entity e) {
            test_assert(e == e1);
            count ++;
        });

    ecs.event(evt)
        .id(id)
        .entity(e1)
        .emit();

    test_int(count, 1);
}

void Event_evt_2_ids_entity() {
    flecs::world ecs;

    auto evt = ecs.entity();
    auto id_a = ecs.entity();
    auto id_b = ecs.entity();
    auto e1 = ecs.entity().add(id_a).add(id_b);

    int32_t count = 0;

    ecs.trigger()
        .event(evt)
        .id(id_a)
        .each([&](flecs::entity e) {
            test_assert(e == e1);
            count ++;
        });

    ecs.trigger()
        .event(evt)
        .id(id_b)
        .each([&](flecs::entity e) {
            test_assert(e == e1);
            count ++;
        });

    ecs.event(evt)
        .id(id_a)
        .id(id_b)
        .entity(e1)
        .emit();

    test_int(count, 2);
}

void Event_evt_1_id_table() {
    flecs::world ecs;

    auto evt = ecs.entity();
    auto id = ecs.entity();
    auto e1 = ecs.entity().add(id);

    auto table = e1.table();

    int32_t count = 0;

    ecs.trigger()
        .event(evt)
        .id(id)
        .each([&](flecs::entity e) {
            test_assert(e == e1);
            count ++;
        });

    ecs.event(evt)
        .id(id)
        .table(table)
        .emit();

    test_int(count, 1);
}

void Event_evt_2_ids_table() {
    flecs::world ecs;

    auto evt = ecs.entity();
    auto id_a = ecs.entity();
    auto id_b = ecs.entity();
    auto e1 = ecs.entity().add(id_a).add(id_b);
    auto table = e1.table();

    int32_t count = 0;

    ecs.trigger()
        .event(evt)
        .id(id_a)
        .each([&](flecs::entity e) {
            test_assert(e == e1);
            count ++;
        });

    ecs.trigger()
        .event(evt)
        .id(id_b)
        .each([&](flecs::entity e) {
            test_assert(e == e1);
            count ++;
        });

    ecs.event(evt)
        .id(id_a)
        .id(id_b)
        .table(table)
        .emit();

    test_int(count, 2);
}

void Event_evt_type() {
    flecs::world ecs;

    auto id = ecs.entity();
    auto e1 = ecs.entity().add(id);

    int32_t count = 0;

    ecs.trigger()
        .event<Evt>()
        .id(id)
        .each([&](flecs::entity e) {
            test_assert(e == e1);
            count ++;
        });

    ecs.event<Evt>()
        .id(id)
        .entity(e1)
        .emit();

    test_int(count, 1);
}

void Event_evt_1_component() {
    flecs::world ecs;

    auto e1 = ecs.entity().add<IdA>();

    int32_t count = 0;

    ecs.trigger()
        .event<Evt>()
        .id<IdA>()
        .each([&](flecs::entity e) {
            test_assert(e == e1);
            count ++;
        });

    ecs.event<Evt>()
        .id<IdA>()
        .entity(e1)
        .emit();

    test_int(count, 1);
}

void Event_evt_2_components() {
    flecs::world ecs;

    auto e1 = ecs.entity().add<IdA>().add<IdB>();

    int32_t count = 0;

    ecs.trigger()
        .event<Evt>()
        .id<IdA>()
        .each([&](flecs::entity e) {
            test_assert(e == e1);
            count ++;
        });

    ecs.trigger()
        .event<Evt>()
        .id<IdB>()
        .each([&](flecs::entity e) {
            test_assert(e == e1);
            count ++;
        });

    ecs.event<Evt>()
        .id<IdA>()
        .id<IdB>()
        .entity(e1)
        .emit();

    test_int(count, 2);
}

struct EvtData {
    int value;
};

void Event_evt_void_ctx() {
    flecs::world ecs;

    auto evt = ecs.entity();
    auto id = ecs.entity();
    auto e1 = ecs.entity().add(id);

    int32_t count = 0;

    ecs.trigger()
        .event(evt)
        .id(id)
        .iter([&](flecs::iter& it) {
            test_assert(it.entity(0) == e1);
            test_int(it.param<EvtData>()->value, 10);
            count ++;
        });

    EvtData data = {10};

    ecs.event(evt)
        .id(id)
        .entity(e1)
        .ctx(&data)
        .emit();

    test_int(count, 1);
}

void Event_evt_typed_ctx() {
    flecs::world ecs;

    auto id = ecs.entity();
    auto e1 = ecs.entity().add(id);

    int32_t count = 0;

    ecs.trigger()
        .event<EvtData>()
        .id(id)
        .iter([&](flecs::iter& it) {
            test_assert(it.entity(0) == e1);
            test_int(it.param<EvtData>()->value, 10);
            count ++;
        });

    ecs.event<EvtData>()
        .id(id)
        .entity(e1)
        .ctx(EvtData{10})
        .emit();

    test_int(count, 1);
}

void Event_evt_implicit_typed_ctx() {
    flecs::world ecs;

    auto id = ecs.entity();
    auto e1 = ecs.entity().add(id);

    int32_t count = 0;

    ecs.trigger()
        .event<EvtData>()
        .id(id)
        .iter([&](flecs::iter& it) {
            test_assert(it.entity(0) == e1);
            test_int(it.param<EvtData>()->value, 10);
            count ++;
        });

    ecs.event<EvtData>()
        .id(id)
        .entity(e1)
        .ctx({10})
        .emit();

    test_int(count, 1);
}
