# 377-final-project

PRESENTATION:
https://drive.google.com/file/d/1588lTaTKFqhR-epEdmphI-SimZx3Z2eN/view?usp=sharing

DOCUMENTATION:

Changes are in my_malloc.cpp

Changes:
In coalesce() function, we no longer pass in a node to start from
Instead, we start from a head using a variable that is a copy of it.
We also do not break at the first block that can not be merged.
This ensures that all adjacent blocks are merged but also makes it so we have to iterate through the whole list.

New function: allocate() function
When we run out of memory, we allocate a new block.
Code to create a new block is very similar to creating a new head node
Instead of assigning this node to the beginning of the list, we assign it to the end of the list
Uses available_memory to check if memory has run out or not.

Mutex changes:
Put a single mutex lock around critical sections of the code
Critical sections were defined as areas where code was accessing shared data or changing values
Could use multiple mutexes, but this implementation only has a single mutex.
