#include "physics.hpp"

#include "components.hpp"
#include "tracing.hpp"

void physics_system(ember_database& entities, float dt, sol::this_state s) {
    auto _visit_token = tracing::push("physics.visit");

    auto lua = sol::state_view(s);

    struct cbody {
        ember_database::ent_id eid;
        component::position* position;
        const component::body* body;
        float left;
        float right;
        float bottom;
        float top;
    };

    auto make_table = [&](const cbody& body) {
        return lua.create_table_with(
            "eid", body.eid,
            "position", body.position,
            "body", body.body,
            "left", body.left,
            "right", body.right,
            "bottom", body.bottom,
            "top", body.top
        );
    };

    struct cregion {
        float left;
        float right;
        float bottom;
        float top;
    };

    auto make_region_table = [&](const cregion& region) {
        return lua.create_table_with(
            "left", region.left,
            "right", region.right,
            "bottom", region.bottom,
            "top", region.top
        );
    };

    auto call_script = [&](const cbody& a, const cbody& b, const cregion& region) -> std::string {
        if (entities.has_component<component::script>(a.eid)) {
            const auto& script = entities.get_component<component::script>(a.eid);
            auto module_name = "actors." + script.name;
            auto file_name = "data/scripts/actors/" + script.name + ".lua";
            auto import = lua.require_file(module_name, file_name);
            if (import.get_type() != sol::type::table) {
                throw std::runtime_error(file_name + " did not return a table");
            }
            auto script_impl = sol::table(import);
            sol::protected_function on_collide = script_impl["on_collide"];
            if (on_collide) {
                auto aa = make_table(a);
                auto bb = make_table(b);
                auto rr = make_region_table(region);
                auto result = on_collide(aa, bb, rr);
                if (!result.valid()) {
                    sol::error err = result;
                    throw std::runtime_error(file_name + " failure: " + err.what());
                }
            }
        }
        return ""; //TODO get this from on_collide
    };

    auto with_vel = [&](ember_database::ent_id eid, auto&& func) {
        if (entities.has_component<component::velocity>(eid)) {
            auto& vel = entities.get_component<component::velocity>(eid).vel;
            func(vel);
        }
    };

    auto resolve_collision = [&](const cbody& a, const cbody& b, const cregion& region) -> std::string {
        auto r1 = call_script(a, b, region);

        if (r1 == "abort") { return "abort"; }

        auto r2 = call_script(b, a, region);

        if (r2 == "abort") { return "abort"; }

        auto w = region.right - region.left;
        auto h = region.top - region.bottom;

        if (a.body->solid && b.body->solid) {
            auto xresponse = 1;
            auto yresponse = 1;

            if (w < h) { xresponse = 2; } else { yresponse = 2; }

            if (!(
                b.left < a.left && a.left < b.right && a.body->edge_left && b.body->edge_right ||
                a.left < b.left && b.left < a.right && b.body->edge_left && a.body->edge_right ||
                b.left < a.right && a.right < b.right && a.body->edge_right && b.body->edge_left ||
                a.left < b.right && b.right < a.right && b.body->edge_right && a.body->edge_left
            )) {
                xresponse = 0;
            }

            if (!(
                b.bottom < a.bottom && a.bottom < b.top && a.body->edge_bottom && b.body->edge_top ||
                a.bottom < b.bottom && b.bottom < a.top && b.body->edge_bottom && a.body->edge_top ||
                b.bottom < a.top && a.top < b.top && a.body->edge_top && b.body->edge_bottom ||
                a.bottom < b.top && b.top < a.top && b.body->edge_top && a.body->edge_bottom
            )) {
                yresponse = 0;
            }

            if (xresponse == 0 && yresponse == 0) { return ""; }

            if (a.body->dynamic) {
                if (xresponse > yresponse) {
                    with_vel(a.eid, [](glm::vec2& vel){ vel.x = 0; });

                    if (b.body->dynamic) {
                        with_vel(b.eid, [](glm::vec2& vel){ vel.x = 0; });

                        if (a.position->pos.x < b.position->pos.x) {
                            a.position->pos.x = a.position->pos.x - w / 2;
                            b.position->pos.x = b.position->pos.x + w / 2;
                        } else {
                            a.position->pos.x = a.position->pos.x + w / 2;
                            b.position->pos.x = b.position->pos.x - w / 2;
                        }
                    } else {
                        if (a.position->pos.x < b.position->pos.x) {
                            a.position->pos.x = a.position->pos.x - w;
                        } else {
                            a.position->pos.x = a.position->pos.x + w;
                        }
                    }
                } else {
                    with_vel(a.eid, [](glm::vec2& vel){ vel.y = 0; });

                    if (b.body->dynamic) {
                        with_vel(b.eid, [](glm::vec2& vel){ vel.y = 0; });

                        if (a.position->pos.y < b.position->pos.y) {
                            a.position->pos.y = a.position->pos.y - h / 2;
                            b.position->pos.y = b.position->pos.y + h / 2;
                        } else {
                            a.position->pos.y = a.position->pos.y + h / 2;
                            b.position->pos.y = b.position->pos.y - h / 2;
                        }
                    } else {
                        if (a.position->pos.y < b.position->pos.y) {
                            a.position->pos.y = a.position->pos.y - h;
                        } else {
                            a.position->pos.y = a.position->pos.y + h;
                        }
                    }
                }
            } else if (b.body->dynamic) {
                if (xresponse > yresponse) {
                    with_vel(b.eid, [](glm::vec2& vel){ vel.x = 0; });

                    if (b.position->pos.x < a.position->pos.x) {
                        b.position->pos.x = b.position->pos.x - w;
                    } else {
                        b.position->pos.x = b.position->pos.x + w;
                    }
                } else {
                    with_vel(b.eid, [](glm::vec2& vel){ vel.y = 0; });

                    if (b.position->pos.y < a.position->pos.y) {
                        b.position->pos.y = b.position->pos.y - h;
                    } else {
                        b.position->pos.y = b.position->pos.y + h;
                    }
                }
            }
        }

        return "";
    };

    auto update_body = [](cbody& bod) {
        bod.left = bod.position->pos.x - bod.body->width / 2;
        bod.right = bod.position->pos.x + bod.body->width / 2;
        bod.bottom = bod.position->pos.y - bod.body->height / 2;
        bod.top = bod.position->pos.y + bod.body->height / 2;
    };

    auto bodies = std::vector<cbody>{};

    {
        auto _gather_token = tracing::push("physics.visit[gather bodies]");
        entities.visit([&](ember_database::ent_id eid, component::position& position, const component::body& body) {
            const auto& pos = position.pos;
            const auto& x = pos.x;
            const auto& y = pos.y;
            const auto& w = body.width / 2;
            const auto& h = body.height / 2;
            bodies.push_back({
                eid,
                &position,
                &body,
                x - w,
                x + w,
                y - h,
                y + h
            });
        });
    }

    {
        auto _sort_token = tracing::push("physics.visit[sort bodies]");
        std::sort(begin(bodies), end(bodies), [](const auto& a, const auto& b){ return a.left < b.left; });
    }

    auto axis_list = std::vector<cbody>{};

    {
        auto _sweep_token = tracing::push("physics.visit[sweep]");
        for (auto& a : bodies) {
            axis_list.erase(
                std::remove_if(begin(axis_list), end(axis_list), [&](const auto& b){ return a.left >= b.right; }),
                end(axis_list));

            for (auto& b : axis_list) {
                    auto region = cregion{
                        std::max(a.left, b.left),
                        std::min(a.right, b.right),
                        std::max(a.bottom, b.bottom),
                        std::min(a.top, b.top),
                    };

                    auto mx = region.right - region.left;
                    auto my = region.top - region.bottom;

                    constexpr auto threshold = 1.f/32.f;

                    if (mx > 0 && my > 0 && (mx > threshold || my > threshold)) {
                        {
                            tracing::push("physics.visit[resolve_collision]");
                            if (resolve_collision(a, b, region) == "abort") { return; }
                        }
                        update_body(a);
                        update_body(b);
                    }
            }

            axis_list.push_back(a);
        }
    }

    {
        tracing::push("physics.visit[border]");
        const auto& scrw = 25.f/2;
        const auto& scrh = 17.25f/2;
        for (const auto& bod : bodies) {
            auto& pos = bod.position->pos;
            const auto& left = bod.left;
            const auto& right = bod.right;
            const auto& bottom = bod.bottom;
            const auto& top = bod.top;

            if (left < -scrw) { pos.x = pos.x + -scrw - left; }
            if (right > scrw) { pos.x = pos.x + scrw - right; }
            if (bottom < -scrh) { pos.y = pos.y + -scrh - bottom; }
            if (top > scrh) { pos.y = pos.y + scrh - top; }
        }
    }
}
