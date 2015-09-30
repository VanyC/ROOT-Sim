#include <ROOT-Sim.h>
#include <math.h>
#include <stdio.h>

#include "application.h"

void initCell(lp_cell *current_cell, int me) {
	//init cell
	current_cell->my_state->id = me;
	current_cell->my_state->sugar_capacity = Random()*100;
	current_cell->my_state->occupied = 0;
	current_cell->my_state->sugar_tank = 0;
	
	current_cell->events = 0;
	current_cell->cell_agent = (agent *) malloc(sizeof(agent));
	bzero(current_cell->cell_agent, sizeof(agent));
	
	int i;
	for(i=0;i<N_DIR; i++) {
		//current_cell->neighbours_state[i] = malloc(sizeof(state_cell));

		//bzero(current_cell->neighbours_state[i], sizeof(state_cell));
		current_cell->neighbours_state[i].id = 0;
		current_cell->neighbours_state[i].sugar_tank = 0;
		current_cell->neighbours_state[i].occupied = 0;
		current_cell->neighbours_state[i].sugar_capacity = 0;
	}
}			

void assignAgent(agent *cell_agent) {
	cell_agent->vision = N_DIR;
	cell_agent->metabolic_rate = Random()*10; 
	cell_agent->max_age = Random()*100;                   
	cell_agent->age = Random()*10;                      
	cell_agent->wealth = Random()*100;
}

void initEvent(event_content_type *new_event_content) {		
	new_event_content->id_cell = -1;
	new_event_content->direction_origin = -1;
	new_event_content->sugar_tank = -1;
	new_event_content->occupied = -1;
	
	new_event_content->vision = -1;                   
	new_event_content->metabolic_rate = -1;           
	new_event_content->max_age = -1;                  
	new_event_content->age = -1;                      
	new_event_content->wealth = -1;	
}

void printCellState(lp_cell *current_cell) {
	printf("CELL Informartion:\n");
	printf("- id: %d\n",current_cell->my_state->id);
	printf("- occupied: %d\n",current_cell->my_state->occupied);
	printf("- capacity: %d\n",current_cell->my_state->sugar_capacity);
	printf("- sugar_tank: %d\n",current_cell->my_state->sugar_tank);
}
/*	
bool isValidNeighbour(unsigned int sender, unsigned int neighbour) {

 	// For hexagon topology
 	unsigned int edge;
 	unsigned int x, y, nx, ny;


	// Convert linear coords to hexagonal coords
	edge = sqrt(n_prc_tot);
	x = sender % edge;
	y = sender / edge;

	// Sanity check!
	if(edge * edge != n_prc_tot) {
		rootsim_error(true, "Hexagonal map wrongly specified!\n");
	}

	// Get the neighbour value
	switch(neighbour) {
		case NW:
			nx = (y % 2 == 0 ? x - 1 : x);
			ny = y - 1;
			break;
		case NE:
			nx = (y % 2 == 0 ? x : x + 1);
			ny = y - 1;
			break;
		case SW:
			nx = (y % 2 == 0 ? x - 1 : x);
			ny = y + 1;
			break;
		case SE:
			nx = (y % 2 == 0 ? x : x + 1);
			ny = y + 1;
			break;
		case E:
			nx = x + 1;
			ny = y;
			break;
		case W:
			nx = x - 1;
			ny = y;
			break;
	}

	if(nx < 0 || ny < 0 || nx >= edge || ny >= edge) {
		return false;
	}

	return true;
}



int GetNeighbourId(unsigned int sender, unsigned int neighbour) {
	unsigned int receiver;

 	// For hexagon topology
 	unsigned int edge;
 	unsigned int x, y, nx, ny;

	// Convert linear coords to hexagonal coords
	edge = sqrt(n_prc_tot);
	x = sender % edge;
	y = sender / edge;

	// Sanity check!
	if(edge * edge != n_prc_tot) {
		rootsim_error(true, "Hexagonal map wrongly specified!\n");
	}

	switch(neighbour) {
		case NW:
			nx = (y % 2 == 0 ? x - 1 : x);
			ny = y - 1;
			break;
		case NE:
			nx = (y % 2 == 0 ? x : x + 1);
			ny = y - 1;
			break;
		case SW:
			nx = (y % 2 == 0 ? x - 1 : x);
			ny = y + 1;
			break;
		case SE:
			nx = (y % 2 == 0 ? x : x + 1);
			ny = y + 1;
			break;
		case E:
			nx = x + 1;
			ny = y;
			break;
		case W:
			nx = x - 1;
			ny = y;
			break;
	}

	// Convert back to linear coordinates
	receiver = (ny * edge + nx);

	return receiver;
}
*/
