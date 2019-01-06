/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//                                  NOTE                                   //
//                                                                         //
//                   You should not change this file!                      //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


#ifndef SHAPES_H
#define SHAPES_H

#include <SFML/Graphics.hpp>


namespace capp {

// Add a shape for this thread and move it onto the screen.
void enter();

// Change this thread's shape color.
void set_color(sf::Color color);

// Move this thread's shape to the destination.
void move_to(sf::Vector2f destination);

// Move this thread's shape off screen, and remove it.
void leave();

// Draw all the shapes on the given window.
void draw_shapes(sf::RenderWindow& window);

};


#endif
