/*
 * usage/main.cpp
 * Q@hackers.pk
 */

/*
 * Deliberately crafted to demonstrate hash collision.
 * Both "hello" and "start" compress to the same bucket index
 * when array_size is 10, illustrating why a collision-handling
 * strategy (chaining or linear probing) is necessary.
 */

#include <iostream>
#include "./../header.hh"

int main()
{
    const size_t BUCKET_COUNT = 10;

    std::string str = "hello";

    size_t index1 = Keys::generate_key(str, BUCKET_COUNT);
    size_t index2 = Keys::generate_key("start", BUCKET_COUNT);

    std::cout << "\"hello\" -> bucket " << index1 << std::endl;
    std::cout << "\"start\" -> bucket " << index2 << std::endl;

    if (index1 == index2)
    {
        std::cout << "\nCollision detected: both strings map to bucket " << index1 << std::endl;
    }

    return 0;
}