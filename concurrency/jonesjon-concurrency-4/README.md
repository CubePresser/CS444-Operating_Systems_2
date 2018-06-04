# Concurrency-4

### Problem 1:

A barbershop consists of a waiting room with n chairs, and the barber room containing the barber chair. If there are no customers to be served, the barber goes to sleep. If a customer enters the barbershop and all chairs are occupied, then the customer leaves the shop. If the barber is busy, but chairs are available, then the customer sits in one of the free chairs. If the barber is asleep, the customer wakes up the barber. Write a program to coordinate the barber and the customers.

Some useful constraints:

* Customers invoke get_hair_cut when sitting in the barber chair.
* If the shop is full, a customer exits the shop.
* The barber thread invokes cut_hair.
* cut_hair and get_hair_cut should always be executing concurrently, for the same duration.
* Your solution should be valid for any number of chairs.

Write concurrent code for customers and barbers that implements a solution to the above constraints.

### Problem 2:

Implement a solution to the cigarette smokers problem, as described in The Little Book of Semaphores, section 4.5.
