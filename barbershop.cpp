#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cmath>
#include <SFML/Graphics.hpp>

#include "shapes.h"

using namespace std::chrono_literals;


const unsigned int WIDTH = 640;
const unsigned int HEIGHT = 480;

const sf::Vector2f BARBER_CHAIR{210, 240};
const sf::Vector2f BARBER_STAND{100, 240};
const sf::Vector2f CUST_CHAIRS[3] = {{430, 120}, {430, 240}, {430, 360}};


// Since we want to turn red while we wait, let's make a function that will
// automatically turn us red, wait, then turn green when we're back.

template <typename Lock, typename Predicate>
void red_wait(std::condition_variable& cv, Lock& lk, Predicate p)
{
    capp::set_color(sf::Color::Red);
    cv.wait(lk, p);
    capp::set_color(sf::Color::Green);
}


// ***************************************************************************
// The first conditions is: "are there waiting customers?" The barber sleeps
// if this condition isn't true. This condition changes when customers sit in
// a waiting chair, and when they get out of a waiting chair.
// ***************************************************************************

std::mutex m_customers_waiting;
std::condition_variable cv_cust_waiting;
bool cust_chair_open[3] = {true, true, true};
int num_cust_waiting = 0;

// if there is a customer waiting...
int how_many_waiting_customers()
{
    std::lock_guard<std::mutex> lk{m_customers_waiting};
    return num_cust_waiting;
}

// Barber sleeps here...
void wait_for_customers()
{
    std::unique_lock<std::mutex> lk{m_customers_waiting};
    red_wait(cv_cust_waiting, lk, []{ return num_cust_waiting > 0;});
}

// A customer looks for a waiting chair. The function returns which
// chair to sit in, or -1 if there are no available chairs.

int get_a_chair()
{
    std::unique_lock<std::mutex> lk{m_customers_waiting};
    if (num_cust_waiting == 3)
        return -1;

    num_cust_waiting++;

    int chair;
    for (chair = 0; chair < 3; chair++)
    {
        if (cust_chair_open[chair])
        {
            cust_chair_open[chair] = false;
            break;
        }
    }

    cv_cust_waiting.notify_one();
    return chair;
}

void leave_waiting_chair(int chair)
{
    std::lock_guard<std::mutex> lk{m_customers_waiting};
    cust_chair_open[chair] = true;
    num_cust_waiting--;
}


// ***************************************************************************
// The next condition: customers wait until the barber is ready.
// ***************************************************************************

std::mutex m_barber_ready;
std::condition_variable cv_barber_ready;
bool barber_ready = true;

// Request for the next customer...
void next_customer()
{
    std::lock_guard<std::mutex> lk{m_barber_ready};
    barber_ready = true;
    cv_barber_ready.notify_one();
}

void wait_for_barber_ready()
{
    std::unique_lock<std::mutex> lk{m_barber_ready};
    red_wait(cv_barber_ready, lk, []{ return barber_ready; });
    barber_ready = false;
}


// ***************************************************************************
// The third condition: the barber waits until the customer is ready before
// starting the haircut.
// ***************************************************************************

std::mutex m_customer_ready;
std::condition_variable cv_customer_ready;
bool customer_ready = false;

// Customer...
void i_am_ready_for_a_haircut()
{
    std::lock_guard<std::mutex> lk{m_customer_ready};
    customer_ready = true;
    cv_customer_ready.notify_one();
}

// Barber...
void are_you_ready_for_a_haircut()
{
    std::unique_lock<std::mutex> lk{m_customer_ready};
    red_wait(cv_customer_ready, lk, []{ return customer_ready; });
    customer_ready = false;
}


// ***************************************************************************
// The fourth condition: the customer waits until their haircut is done to
// get out of the barber chair.
// ***************************************************************************

std::mutex m_haircut_done;
std::condition_variable cv_haircut_done;
bool haircut_done = false;

// Barber...
void your_haircut_is_done()
{
    std::lock_guard<std::mutex> lk{m_haircut_done};
    haircut_done = true;
    cv_haircut_done.notify_one();
}

// Customer...
void wait_for_haircut_done()
{
    std::unique_lock<std::mutex> lk{m_haircut_done};
    red_wait(cv_haircut_done, lk, []{ return haircut_done; });
    haircut_done = false;
}



void cut_hair()
{
    capp::set_color(sf::Color::Yellow);
    std::this_thread::sleep_for(2s);
    capp::set_color(sf::Color::Green);
}


void barber_thread()
{
    capp::enter();
    capp::move_to(BARBER_STAND);

    while (true)
    {
        if (how_many_waiting_customers() == 0)
        {
            capp::move_to(BARBER_CHAIR);
            wait_for_customers();
            capp::move_to(BARBER_STAND);
        }

        next_customer();
        are_you_ready_for_a_haircut();
        cut_hair();
        your_haircut_is_done();
    }
}


void customer_thread()
{
    capp::enter();

    int mychair = get_a_chair();
    if (mychair == -1)
    {
        capp::leave();
        return;
    }
    
    capp::move_to(CUST_CHAIRS[mychair]);
    wait_for_barber_ready();
    leave_waiting_chair(mychair);
    capp::move_to(BARBER_CHAIR);
    i_am_ready_for_a_haircut();
    wait_for_haircut_done();
    capp::leave();
}

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//                                  NOTE                                   //
//                                                                         //
// All of your changes should be above this note! You will add global      //
// variables, mutexes, and condition variables. You will use them in the   //
// barber and customer threads. You will only use the capp namespace       //
// functions (from shapes.h) to manipulate the shapes!                     //
/////////////////////////////////////////////////////////////////////////////


void setup_chairs(std::vector<sf::RectangleShape>&);


int main()
{
    sf::RenderWindow window{sf::VideoMode(WIDTH, HEIGHT), "Barbershop"};
    window.setFramerateLimit(60);

    std::thread barber{barber_thread};
    barber.detach();

    std::vector<sf::RectangleShape> chairs;
    setup_chairs(chairs);

    while (window.isOpen())
    {
        sf::Event event;
        while(window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
            else if (event.type == sf::Event::KeyPressed)
            {
                std::thread customer{customer_thread};
                customer.detach();
            }
        }

        window.clear(sf::Color::White);
        for (auto chair: chairs)
            window.draw(chair);     
        capp::draw_shapes(window);
        window.display();    
    }

    return 0;
}


void setup_chairs(std::vector<sf::RectangleShape>& chairs)
{
    sf::RectangleShape chair;
    chair.setFillColor({200, 200, 200});
    chair.setSize({40, 40});
    chair.setOrigin({20, 20});

    for (auto position: CUST_CHAIRS)
    {
        chair.setPosition(position);
        chairs.push_back(chair);    
    }

    chair.setPosition(BARBER_CHAIR);
    chairs.push_back(chair);
}
