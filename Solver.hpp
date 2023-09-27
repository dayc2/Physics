//
// Created by Cameron Day on 3/19/23.
//

#include "SFML/Graphics.hpp"
#include <cmath>
#include <thread>
#include "ThreadPool.hpp"
#include "CollisionGrid.hpp"

using sf::Vector2f;

struct VerletObject{

    Vector2f pos;
    Vector2f pos_prev;
    Vector2f acc;
    float radius;
    sf::Color color;
    int polarity{};


    VerletObject() = default;
    VerletObject(Vector2f pos_, float radius_):
    pos{pos_},
    radius{radius_},
    pos_prev{pos_},
    acc{0.0f, 0.0f},
    color{sf::Color::Blue},
    polarity{1}
    {}

    VerletObject(Vector2f pos_, float radius_, sf::Color color_):
    pos{pos_},
    radius{radius_},
    pos_prev{pos_},
    acc{0.0f, 0.0f},
    color{color_},
    polarity{1}
    {}

    VerletObject(Vector2f pos_, float radius_, int polarity_):
    pos{pos_},
    radius{radius_},
    pos_prev{pos_},
    acc{0.0f, 0.0f},
    color{},
    polarity{polarity_}
    {if(polarity_ > 0)
        color = sf::Color::Blue;
    else
        color = sf::Color::Red;
    }


    void update(float dt){
        Vector2f dis = pos-pos_prev;
        pos_prev = pos;
        pos += dis + acc * (dt * dt);
        acc = {};
    }

    void accelerate(Vector2f a){
        acc += a;
    }

};

class Solver{
public:

    Vector2f worldSize;
    std::vector<VerletObject> objects;

    Solver() = delete;
    explicit Solver(Vector2f size, ThreadPool& threadPool_):
    grid{static_cast<int32_t>(size.x), static_cast<int32_t>(size.y)},
    worldSize{size.x, size.y},
    threadPool{threadPool_}{
        grid.clear();
    }


    uint32_t addObject(Vector2f pos, float radius){
        objects.emplace_back(pos, radius);
        applySingleConstraint(objects.back());
        objects.back().pos_prev = objects.back().pos;
        return objects.size()-1;
    }

    uint32_t addObject(Vector2f pos, float radius, sf::Color color){
        objects.emplace_back(pos, radius, color);
        applySingleConstraint(objects.back());
        objects.back().pos_prev = objects.back().pos;
        return objects.size()-1;
    }

//    void addObject(Vector2f pos, float radius, int polarity){
//        objects.emplace_back(pos, radius, polarity);
//        applySingleConstraint(objects.back());
//        objects.back().pos_prev = objects.back().pos;
//    }

    void setStep(float s){
        step = s;
        dt = s/substep;
    }

    void setSubstep(int s){
        substep = s;
        dt = step / substep;
    }

    void update(){
        for(uint i = 0; i < substep; i++) {
            applyGravity();
            applyConstraints();
            solveCollisions();
            updateObjects();
        }
    }

    [[nodiscard]]
    const std::vector<VerletObject>& getObjects() const{
        return objects;
    }


private:
    Vector2f gravity = {0.0f, 20.0f};
    float dt{};
    float step{};
    uint substep = 8;
    float friction = 1.0f;
    ThreadPool& threadPool;
    CollisionGrid grid;

    void updateObjects(){
        threadPool.dispatch(objects.size(), [&](uint32_t start, uint32_t end){
            for(uint32_t i{start}; i < end; i++){
                VerletObject& v1 = objects[i];
                v1.update(dt);
            }
        });
    }

    void applyConstraints(){
        threadPool.dispatch(objects.size(), [&](uint32_t start, uint32_t end){
            for(uint32_t i{start}; i < end; i++){
                VerletObject& v1 = objects[i];
                applySingleConstraint(v1);
            }
        });
    }

    void applyGravity(){
        threadPool.dispatch(objects.size(), [&](uint32_t start, uint32_t end){
            for(uint32_t i{start}; i < end; i++){
                VerletObject& v1 = objects[i];
//                applyMagnetismSingle(v1);
                v1.accelerate(gravity);
            }
        });
    }


    void applyMagnetismSingle(VerletObject& v1) {
        Vector2f forces{0.0f, 0.0f};
        for (VerletObject &v: objects) {
            Vector2f pos = v.pos - v1.pos;
            float dist = pos.x * pos.x + pos.y * pos.y;
            if (pos.x != 0 && pos.y != 0 && dist*dist < 1000.0f) {
                dist = sqrt(pos.x * pos.x + pos.y * pos.y);
                pos = pos / dist;
                float m1{v1.radius};
                float m2{v.radius};
                forces += 1000.0f * m1 * m2 * pos / (dist * dist);
            }
        }
        v1.accelerate(forces);

    }


    void applySingleConstraint(VerletObject& v){
        const float margin = 2.0f;
        if(v.pos.x > worldSize.x - margin)
            v.pos.x = worldSize.x - margin;
        else if(v.pos.x < margin)
            v.pos.x = margin;
        if(v.pos.y > worldSize.y - margin)
            v.pos.y = worldSize.y - margin;
        else if(v.pos.y < margin)
            v.pos.y = margin;
    }

    void solveCollisions(){
//        for(uint i = 0; i < objects.size(); i++){
//            for(uint j = i+1; j < objects.size(); j++){
//                solveCollision(objects[i], objects[j]);
//            }
//        }

        addObjectsToGrid();
        const uint32_t thread_count = threadPool.thread_count;
        const uint32_t slice_count = thread_count * 2;
        const uint32_t slice_size = (grid.width / slice_count) * grid.height;

        for(uint32_t i{0}; i < thread_count; i++){
            threadPool.addTask([this, i, slice_size]{
                solveCollisions(2 * i, slice_size);
            });
        }
        threadPool.waitForCompletion();

        for(uint32_t i{0}; i < thread_count; i++){
            threadPool.addTask([this, i, slice_size]{
                solveCollisions(2 * i + 1, slice_size);
            });
        }
        threadPool.waitForCompletion();


    }

    void solveCollision(VerletObject& v1, VerletObject& v2){
        Vector2f pos = v1.pos - v2.pos;
        float dist = (pos.x) * (pos.x) + (pos.y) * (pos.y);
//        float min = v1.radius + v2.radius;
        if(dist < 1.0f){
            dist = sqrt(dist);
            if(dist != 0) {
                Vector2f n = pos / dist;
//                float v1_ratio = v1.radius / (v1.radius + v2.radius);
//                float v2_ratio = v2.radius / (v1.radius + v2.radius);
                float offset = 0.5f * friction * (dist - 1.0f);
                v1.pos -= n * offset;
                v2.pos += n * offset;
            }
        }
    }

    void solveCollisions(uint32_t i, uint32_t slice_size){
        const uint32_t start = i * slice_size;
        const uint32_t end = (i+1) * slice_size;
        for(uint32_t index{start}; index < end; index++){

            solveCell(grid.data[index], index);
        }
    }

    void solveCell(Cell &cell, uint32_t index) {
        for(uint32_t i{0}; i < cell.objects_count; i++){
            const uint32_t object = cell.objects[i];
            solveCellCollision(object, grid.data[index - 1]);
            solveCellCollision(object, grid.data[index]);
            solveCellCollision(object, grid.data[index + 1]);
            solveCellCollision(object, grid.data[index + grid.height - 1]);
            solveCellCollision(object, grid.data[index + grid.height]);
            solveCellCollision(object, grid.data[index + grid.height + 1]);
            solveCellCollision(object, grid.data[index - grid.height - 1]);
            solveCellCollision(object, grid.data[index - grid.height]);
            solveCellCollision(object, grid.data[index - grid.height + 1]);
        }
    }

    void solveCellCollision(uint32_t index, const Cell& cell){
        for(uint32_t i{0}; i < cell.objects_count; i++){
            solveCollision(objects[index], objects[cell.objects[i]]);
        }
    }

    void addObjectsToGrid(){
        grid.clear();
        uint32_t i = 0;
//        for(VerletObject& v : objects){
//            if(v.pos.x > 1.0f && v.pos.x < worldSize.x - 1.0f && v.pos.y > 1.0f && v.pos.y < worldSize.y - 1.0f){
//                grid.add(static_cast<int32_t>(v.pos.x), static_cast<int32_t>(v.pos.y), i);
//            }
//            else{
//                std::cout << "problem" << std::endl;
//            }
//            i++;
//        }

        threadPool.dispatch(objects.size(), [&](uint32_t start, uint32_t end){
            for(uint32_t i{start}; i < end; i++){
                VerletObject& v = objects[i];
                if(v.pos.x > 1.0f && v.pos.x < worldSize.x - 1.0f && v.pos.y > 1.0f && v.pos.y < worldSize.y - 1.0f){
                    grid.add(static_cast<int32_t>(v.pos.x), static_cast<int32_t>(v.pos.y), i);
                }
            }
        });
    }
};