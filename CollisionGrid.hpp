//
// Created by Cameron Day on 3/22/23.
//

#pragma once

#include "Grid.hpp"

struct Cell {
    static constexpr uint32_t capacity = 4;
    static constexpr uint32_t max_cell_xid = capacity -1;

    uint32_t objects_count = 0;
    uint32_t objects[capacity] = {};

    Cell() = default;

    void add(uint32_t id){
        objects [objects_count] = id;
        objects_count += objects_count < max_cell_xid;
    }

    void clear(){
        objects_count = 0;
    }

    void remove(uint32_t id){
        for(uint32_t i = 0; i < objects_count; i++){
            if(objects[i] == id){
                objects[i] = objects[objects_count-1];
                objects_count--;
                return;
            }
        }
    }
};

struct CollisionGrid : public Grid<Cell>{
    CollisionGrid():
    Grid<Cell>(){}

    CollisionGrid(int32_t width_, int32_t height_):
    Grid<Cell>(width_, height_){}

    bool add(uint32_t x, uint32_t y, uint32_t object){
        const uint32_t id = x * height + y;
        data[id].add(object);
        return true;
    }

    void clear(){
        for(auto& c : data) c.objects_count = 0;
    }
};