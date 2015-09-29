#pragma once
#ifndef _SSCA_H
#define _SSCA_H

#include <ROOT-Sim.h>

#define UNIFORME	0

//Every step the behavior of the agents should be updated 
#define TIME_STEP 1.0

// EVENT TYPES
#define CELL_IN	        1          //an agent arrives to a cell 
#define CELL_OUT	        2          //an agent leaves a cell
#define UPDATE_NEIGHBORS	3          //update the states of its neighbours 
#define UPDATE_ME         4          //every step a cell sould update its own state

//DIRECTIONS 
#define N_DIR 4
#define N	0
#define S	1
#define E	2
#define W	3

#define ALPHA 3.5 
#ifndef NUM_CELL_OCCUPIED
	#define NUM_CELL_OCCUPIED	2
#endif

#ifndef MAX_EVENTS
	#define MAX_EVENTS	1000
#endif

typedef struct _event_content_type {
	//about the cell
	int id_cell;
	int direction_origin;
	int sugar_tank;
	bool occupied;
	//about the agent of the cell 
	int vision;                   
	int metabolic_rate;           
	int max_age;                  
	int age;                      
	int wealth;                   
} event_content_type;

typedef struct _agent{
	int vision;                   // The max # of cells an agent can see
	int metabolic_rate;            // The # of sugar an agent can burn in every step
	int max_age;                   // Maxim age an agent can live
	int age;                      // In month, age of the agent
	int wealth;                   // Capacity to accumulate sugar, if collect -> wealth+=sugar; if metabolicRate -> wealth-=metabolicRate; if wealth<=0 -> die
} agent;

typedef struct _state_cell{
	int id;
	int sugar_tank;
	bool occupied;
	int sugar_capacity;
} state_cell;

typedef struct _lp_cell{
	int events;
	state_cell *my_state; //my state
	agent *cell_agent;
	struct _state_cell neighbours_state[4]; //state of my neigbours
} lp_cell;

//-----------------------------------------
//           FUNCTIONS
//-----------------------------------------
//------INIT--------
void initCell(lp_cell *current_cell, int me);
void assignAgent(agent *cell_agent);
void initEvent(event_content_type *new_event_content);
//------Behavior of neighbours
//int findNeighbour(unsigned int sender);
//bool isValidNeighbour(unsigned int sender, unsigned int neighbour);
//int GetNeighbourId(unsigned int sender, unsigned int neighbour);


#endif /* _ANT_ROBOT_H */
