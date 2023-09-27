#include <iostream>
#include <random>
#include "SFML/Graphics.hpp"
#include "Solver.hpp"
#include "ThreadPool.hpp"
#include <fstream>

int main() {

    sf::Image image;
    sf::Texture t;
    t.loadFromFile("/Users/cameron/Desktop/CProjects/Physics/turtle.jpg");
    if(!image.loadFromFile("/Users/cameron/Desktop/CProjects/Physics/turtle.jpg")){
        std::cout << "image not found\n";
        return 1;
    }
    sf::Color pixel = image.getPixel(0, 0);

    ThreadPool pool{2};
    Vector2f worldSize{100.0f, 100.0f};
    Solver solver(worldSize, pool);
    int framerate = 60;
    float dt = 1.0f/framerate;
    solver.setStep(dt);
    solver.setSubstep(8);

    float scale = image.getSize().x / worldSize.x;
    std::cout << scale;

    const uint32_t window_width  = 1500;
    const uint32_t window_height = 1500;
    sf::RenderWindow window(sf::VideoMode(window_width, window_height), "Verlet Simulation");
    sf::CircleShape shape;
    const float margin = 20.0f;
//    const auto  zoom   = static_cast<float>(window_height - margin) / static_cast<float>(worldSize.y);
//    render_context.setZoom(zoom);
//    render_context.setFocus({worldSize.x * 0.5f, worldSize.y * 0.5f});
    window.setFramerateLimit(framerate);

//    solver.addObject({100.0f, 200.0f}, 1.0f, 1);
    float input_size = 10.0f;

    sf::Font font;

    auto time = std::chrono::steady_clock::now();

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> pos_dist(0, worldSize.x*2+1);
    std::uniform_real_distribution<float> size_dist(10.0f, 20.0f);

    if(!font.loadFromFile("/Users/cameron/Desktop/CProjects/Physics/roboto/Roboto-Regular.ttf")){
        return 1;
    }
    double timer = 0;


    std::ifstream fileStream;
    fileStream.open("/Users/cameron/Desktop/CProjects/Physics/ImagePixels.txt");
    while (window.isOpen())
    {
        time = std::chrono::steady_clock::now();
        uint size = solver.objects.size();
        if(size < 10000){
            for(uint32_t i{size/1000 + 1}; i--;){
                uint32_t color;
//                if(!fileStream.eof())
                fileStream >> color;
                uint32_t id = solver.addObject({2.0f, 10.0f + i * 2.0f}, 1.0f, sf::Color(color));
                solver.objects[id].pos_prev.x -= 0.2f;
//                solver.objects[id].pos_prev.y -= .02f;
            }
        }else {
//            for(VerletObject& v : solver.objects) {
//                v.color = image.getPixel(int(v.pos.x * scale), int(v.pos.y * scale));
//            }
        }

        solver.update();



        window.clear();
//        shape.setPointCount(1000);
//        window.draw(shape);
//        pool.dispatch(size, [&](uint32_t start, uint32_t end) {
//        for(uint32_t i{start}; i < end; i++) {
//            shape.setRadius(window_width/worldSize.x/2);
//            VerletObject& v = solver.objects[i];
//            shape.setPosition((v.pos.x- v.radius)*window_width/worldSize.x, (v.pos.y - v.radius)*window_height/worldSize.y);
//            shape.setFillColor(v.color);
//            window.draw(shape);
//        }
//        });

        for(uint32_t i{0}; i < size; i++) {
            shape.setRadius(window_width/worldSize.x/2);
            VerletObject& v = solver.objects[i];
            shape.setPosition((v.pos.x- v.radius)*window_width/worldSize.x, (v.pos.y - v.radius)*window_height/worldSize.y);
            shape.setFillColor(v.color);
            shape.setTexture(&t);
            window.draw(shape);
        }

        sf::Event event{};
        if(window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            if (timer == 0 && sf::Mouse::isButtonPressed(sf::Mouse::Left)){
                auto mouse_pos = sf::Mouse::getPosition(window);
                solver.addObject({float(mouse_pos.x)/window_width*worldSize.x, float(mouse_pos.y)/window_height*worldSize.y}, 1.0f);
                std::cout << event.mouseButton.x << " " << event.mouseButton.y << std::endl;
                timer = 15;
            }else if(timer == 0 && sf::Mouse::isButtonPressed(sf::Mouse::Right)){
                auto mouse_pos = sf::Mouse::getPosition(window);
                solver.addObject({float(mouse_pos.x)/window_width*worldSize.x, float(mouse_pos.y)/window_height*worldSize.y}, 1.0f);
                std::cout << float(event.mouseButton.x)/window_width*worldSize.x << " " << float(event.mouseButton.y)/window_height*worldSize.y << std::endl;
                timer = 15;
            }
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)){
                input_size = 10.0f;
            }else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)){
                input_size = 20.0f;
            }else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)){
                input_size = 30.0f;
            }
            if(timer > 0) timer--;
        }

        std::string s = std::to_string((std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - time)).count());
        sf::Text time_text;
        sf::Text objects_text;
        time_text.setFont(font);
        objects_text.setFont(font);
        objects_text.setPosition(time_text.getPosition().x, time_text.getPosition().y + 30);
        std::string stime = std::to_string(((std::chrono::steady_clock::now() - time)/1000000.0).count());
        time_text.setString("Delay: " + stime + "ms");
        objects_text.setString("Objects: " + std::to_string(size));
        std::chrono::steady_clock::now();

        window.draw(time_text);
        window.draw(objects_text);
        window.display();
    }

    std::ofstream s;
    s.open("/Users/cameron/Desktop/CProjects/Physics/ImagePixels.txt");
    if(!s.good()){
        std::cout << "imagePixels not open\n";
        return 1;
    }
    for(VerletObject& v : solver.objects){
        s << image.getPixel(int(v.pos.x * scale), int(v.pos.y * scale)).toInteger() << " ";
    }
    fileStream.close();
    s.close();
    return 0;
}
