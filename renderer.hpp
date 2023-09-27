//
// Created by Cameron Day on 3/23/23.
//
#pragma once
#include <SFML/Graphics.hpp>
#include "Solver.hpp"
#include "WindowContextHandler.hpp"


struct Renderer
{
    Solver& solver;

    sf::VertexArray world_va;
    sf::VertexArray objects_va;
    sf::Texture     object_texture;

    ThreadPool& thread_pool;

    explicit
    Renderer(Solver& solver_, ThreadPool& tp);

    void render(RenderContext& context);

    void initializeWorldVA();

    void updateParticlesVA();

    void renderHUD(RenderContext& context);
};

Renderer::Renderer(Solver& solver_, ThreadPool& tp)
        : solver{solver_}
        , world_va{sf::Quads, 4}
        , objects_va{sf::Quads}
        , thread_pool{tp}
{
    initializeWorldVA();

    object_texture.loadFromFile("res/circle.png");
    object_texture.generateMipmap();
    object_texture.setSmooth(true);
}

void Renderer::render(RenderContext& context)
{
    renderHUD(context);
    context.draw(world_va);

    sf::RenderStates states;
    states.texture = &object_texture;
    context.draw(world_va, states);
    // Particles
    updateParticlesVA();
    context.draw(objects_va, states);
}

void Renderer::initializeWorldVA()
{
    world_va[0].position = {0.0f               , 0.0f};
    world_va[1].position = {solver.worldSize.x, 0.0f};
    world_va[2].position = {solver.worldSize.x, solver.worldSize.y};
    world_va[3].position = {0.0f               , solver.worldSize.y};

    const uint8_t level = 50;
    const sf::Color background_color{level, level, level};
    world_va[0].color = background_color;
    world_va[1].color = background_color;
    world_va[2].color = background_color;
    world_va[3].color = background_color;
}

void Renderer::updateParticlesVA()
{
    objects_va.resize(solver.objects.size() * 4);

    const float texture_size = 1024.0f;
    const float radius       = 0.5f;
    thread_pool.dispatch(static_cast<uint32_t>(solver.objects.size()), [&](uint32_t start, uint32_t end) {
        for (uint32_t i{start}; i < end; ++i) {
            const VerletObject& object = solver.objects[i];
            const uint32_t idx = i << 2;
            objects_va[idx + 0].position = object.pos + Vector2f{-radius, -radius};
            objects_va[idx + 1].position = object.pos + Vector2f { radius, -radius};
            objects_va[idx + 2].position = object.pos + Vector2f { radius,  radius};
            objects_va[idx + 3].position = object.pos + Vector2f {-radius,  radius};
            objects_va[idx + 0].texCoords = {0.0f        , 0.0f};
            objects_va[idx + 1].texCoords = {texture_size, 0.0f};
            objects_va[idx + 2].texCoords = {texture_size, texture_size};
            objects_va[idx + 3].texCoords = {0.0f        , texture_size};

            const sf::Color color = object.color;
            objects_va[idx + 0].color = color;
            objects_va[idx + 1].color = color;
            objects_va[idx + 2].color = color;
            objects_va[idx + 3].color = color;
        }
    });
}

void Renderer::renderHUD(RenderContext&)
{
    // HUD
    /*const float margin    = 20.0f;
    float       current_y = margin;
    text_time.setString("Simulation time: " + toString(phys_time.get()) + "ms");
    text_time.setPosition({margin, current_y});
    current_y += text_time.getBounds().y + 0.5f * margin;
    context.renderToHUD(text_time, RenderContext::Mode::Normal);

    text_objects.setString("Objects: " + toString(simulation.solver.objects.size()));
    text_objects.setPosition({margin, current_y});
    current_y += text_objects.getBounds().y + 0.5f * margin;
    context.renderToHUD(text_objects, RenderContext::Mode::Normal);*/
}