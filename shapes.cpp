/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//                                  NOTE                                   //
//                                                                         //
//                   You should not change this file!                      //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

#include "shapes.h"

#include <iostream>
#include <cmath>
#include <thread>
#include <mutex>
#include <chrono>

#include <SFML/Graphics.hpp>


constexpr float speed = 125;
constexpr float radius = 15;

const sf::Vector2f PRE_ENTRANCE{670, -30};
const sf::Vector2f ENTRANCE{610, 30};
const sf::Vector2f EXIT{610, 450};
const sf::Vector2f POST_EXIT{670, 510};


std::mutex mutex_shapes;
std::map<std::thread::id, sf::CircleShape> shapes;


void capp::enter()
{
    auto id = std::this_thread::get_id();

    std::unique_lock<std::mutex> lk(mutex_shapes);
    bool already_there = (shapes.count(id) != 0);
    lk.unlock();

    if (already_there)
    {
        std::cerr << "A thread tried to enter a second time!" << std::endl;
        return;
    }

    lk.lock();
    shapes[id].setRadius(radius);
    shapes[id].setOrigin(radius, radius);
    shapes[id].setOutlineColor(sf::Color::Blue);
    shapes[id].setOutlineThickness(2.0);
    shapes[id].setFillColor(sf::Color::Green);
    shapes[id].setPosition(PRE_ENTRANCE);
    lk.unlock();

    move_to(ENTRANCE);
}


void capp::move_to(sf::Vector2f destination)
{
    using namespace std::chrono;

    auto id = std::this_thread::get_id();

    std::unique_lock<std::mutex> lk(mutex_shapes);
    sf::Vector2f beginning = shapes[id].getPosition();
    lk.unlock();

    const sf::Vector2f path = destination - beginning;
    const float length = std::hypot(path.x, path.y);
    auto duration = std::chrono::duration<float>(length / speed);
    auto start = steady_clock::now();
    auto end = start + duration;

    for (auto now = steady_clock::now(); now < end; now = steady_clock::now())
    {
        float ratio_complete = (now - start) / duration;
        sf::Vector2f position = beginning + path * ratio_complete;

        lk.lock();
        shapes[id].setPosition(position);
        lk.unlock();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    lk.lock();
    shapes[id].setPosition(destination);
    lk.unlock();
}


void capp::set_color(sf::Color color)
{
    auto id = std::this_thread::get_id();

    std::lock_guard<std::mutex> lk(mutex_shapes);
    shapes[id].setFillColor(color);
}


void capp::leave()
{
    auto id = std::this_thread::get_id();

    std::unique_lock<std::mutex> lk(mutex_shapes);
    bool not_there = (shapes.count(id) == 0);
    lk.unlock();

    if (not_there)
    {
        std::cerr << "A thread tried to leave, but it wasn't there!" << std::endl;
        return;
    }

    move_to(EXIT);
    move_to(POST_EXIT);

    lk.lock();
    shapes.erase(id);
}


void capp::draw_shapes(sf::RenderWindow& window)
{
    std::unique_lock<std::mutex> lk(mutex_shapes);
    for (const auto& keyval: shapes)
        window.draw(keyval.second);
}
