/*
 * main.cpp
 * Q@hacker.pk
 */

#include <iostream> 
#include "header.hh"

int main() 
{   
    
    cc_tokenizer::String<char> str("hello");
    
    std::cout << keys::generate_key(str, 10) << std::endl;

    std::cout << keys::generate_key("start", 10) << std::endl;
   
    return 0;
}