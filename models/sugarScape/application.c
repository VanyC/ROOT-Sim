#include <ROOT-Sim.h>
#include <stdio.h>
#include <limits.h>

#include "application.h"

//TODO: end condition is not implemented yet
//TODO: how can i used a distribution between [1,6] for ex
//TODO: parametrization is needed
//TODO: how long the model is working

void ProcessEvent(int me, simtime_t now, int event_type, event_content_type *event_content, int event_size, lp_cell *current_cell) {

	event_content_type new_event_content;
	
	initEvent(&new_event_content);
	
	int i;
	int receiver;
	int direction_receiver = -1;
	
	simtime_t timestamp = 0;

	switch(event_type) {

		case INIT: 

			current_cell = (lp_cell *)malloc(sizeof(lp_cell));
			
			if(current_cell == NULL){
				rootsim_error(true, "%s:%d: Unable to allocate memory!\n", __FILE__, __LINE__);
			}
			current_cell->my_state = (state_cell *)malloc(sizeof(state_cell));

			SetState(current_cell);
			
			if(NUM_CELL_OCCUPIED > n_prc_tot){
				rootsim_error(true, "%s:%d: Require more cell than available LPs\n", __FILE__, __LINE__);
			}
			
			initCell(current_cell, me);
							
			//first it is necessary to send this event in order the sugar could grows
			timestamp = now + (simtime_t) (TIME_STEP);
			ScheduleNewEvent(me, timestamp, UPDATE_ME, NULL, 0);
			
			//create the CELL_IN event for the first time 
			new_event_content.id_cell = me;
			new_event_content.direction_origin = -1; //because is the first time
			new_event_content.occupied = 0;
			
			timestamp = now + (simtime_t)(TIME_STEP * Random());
			
			if((NUM_CELL_OCCUPIED % 2) == 0){
				//First and last cell have to be occuped 
				if(me < (NUM_CELL_OCCUPIED/2) || me >= ((n_prc_tot)-(NUM_CELL_OCCUPIED/2))) {
						// A CELL_IN event is been generate
					ScheduleNewEvent(me, timestamp, CELL_IN, &new_event_content, sizeof(new_event_content));
				}
			} else {
				if(me <= (NUM_CELL_OCCUPIED / 2) || me >= ((n_prc_tot) - (NUM_CELL_OCCUPIED / 2))){
						// A CELL_IN event is been generate
					ScheduleNewEvent(me, timestamp, CELL_IN, &new_event_content, sizeof(new_event_content));
				}
			}

			break;


		case CELL_IN:
			current_cell->events++;
			
			//Is the first time, te cell is empty => we need a new agent in that cell
			if(event_content->occupied == 0 && event_content->id_cell == me ) { 
				assignAgent(current_cell->cell_agent);
			}
			else { //Is a migration from a neighbour
				current_cell->cell_agent->vision = event_content->vision; 
				current_cell->cell_agent->metabolic_rate = event_content->metabolic_rate;     
				current_cell->cell_agent->max_age = event_content->max_age;            
				current_cell->cell_agent->age = event_content->age;                
				current_cell->cell_agent->wealth = event_content->wealth;             
			}
			//When an agent arrives at a new cell, eats the sugar of it
			current_cell->cell_agent->wealth+= current_cell->my_state->sugar_tank;
			current_cell->my_state->sugar_tank = 0;
			
			//Everytime a cell has a change (in relation with its agent), the cell communicates it at his neigbours
			new_event_content.id_cell = me;
            new_event_content.sugar_tank = current_cell->my_state->sugar_tank;
			new_event_content.occupied = 1;
			//for this event, it is not important the rest of the params of the event, maybe it will be better if we have two types of events, more clean
			new_event_content.vision = new_event_content.metabolic_rate = new_event_content.max_age = new_event_content.age = new_event_content.wealth = -1; 
			
			for (i = 0; i < N_DIR; i++) {
				receiver = current_cell->neighbours_state[i].id;
				
				switch (i){
					case N: //updating northern neighbour, i am coming from south
						new_event_content.direction_origin = S;
					 break;
					case S: //updating southern neighbour, i am coming from north
						new_event_content.direction_origin = N;
					break;
					case E: 
						new_event_content.direction_origin = W;
					break;
					case W: 
						new_event_content.direction_origin = E;
					break;
				}
				timestamp = now + (simtime_t) (TIME_STEP);
				receiver = GetReceiver(TOPOLOGY_TORUS, direction_receiver); //direction
				ScheduleNewEvent(receiver, timestamp, UPDATE_NEIGHBORS, &new_event_content, sizeof(new_event_content));
			}

			// A CEL_OUT event is been generated
			timestamp = now + (simtime_t) (TIME_STEP * Random());
			ScheduleNewEvent(me, timestamp, CELL_OUT, NULL, 0);

			break;

		case UPDATE_ME:
		
			if(current_cell->my_state->occupied) { //if the cell is occupied, the agent in that cell loses wealth in a metabolicRate proportion
				current_cell->cell_agent->wealth-=current_cell->cell_agent->metabolic_rate;
				
				if(current_cell->cell_agent->wealth <= 0) { //agent is dead => it is necessary to grown a new agent
					new_event_content.id_cell = me;
					new_event_content.sugar_tank = current_cell->my_state->sugar_tank;
					new_event_content.occupied = 0;	
					
					timestamp= now + (simtime_t) (TIME_STEP);
					ScheduleNewEvent(me, timestamp, CELL_IN, &new_event_content, sizeof(new_event_content));			
				}
			}

			int sugar = current_cell->my_state->sugar_tank + ALPHA;
			
			if(sugar <= current_cell->my_state->sugar_capacity) current_cell->my_state->sugar_tank = sugar;
			else  current_cell->my_state->sugar_tank = current_cell->my_state->sugar_capacity;
			
			timestamp= now + (simtime_t) (TIME_STEP);
			ScheduleNewEvent(me, timestamp, UPDATE_ME, NULL, 0);

			break;
			
		case UPDATE_NEIGHBORS:

			current_cell->neighbours_state[event_content->direction_origin].sugar_tank = event_content->sugar_tank;
			current_cell->neighbours_state[event_content->direction_origin].occupied = event_content->occupied;

			break;

		case CELL_OUT: //PF
			
			current_cell->events++;
			// Go to the neighbour who has the biggest sugarTank
			int max_sugar_cell = 0;
			
			for(i = 0; i < N_DIR; i++) {
				//if in that cell there are sugar and it is not occupied
				if(current_cell->neighbours_state[i].sugar_tank > max_sugar_cell && !current_cell->neighbours_state[i].occupied) {
					//The current cell eats the sugar of the new cell
					receiver = current_cell->neighbours_state[i].id;
					direction_receiver = i;
					max_sugar_cell = current_cell->neighbours_state[i].sugar_tank;
					
				}
			}
			
			new_event_content.id_cell = me; //this is just to indicate that is not a new agent, just a migration
			new_event_content.direction_origin = -1; // TODO? Who is my direction
			new_event_content.occupied = 1;
			
			new_event_content.vision = current_cell->cell_agent->vision; 
			new_event_content.metabolic_rate = current_cell->cell_agent->metabolic_rate;     
			new_event_content.max_age = current_cell->cell_agent->max_age;            
			new_event_content.age = current_cell->cell_agent->age;                
			new_event_content.wealth = current_cell->cell_agent->wealth; 
					
			receiver = GetReceiver(TOPOLOGY_TORUS, direction_receiver); //direction		
					
			timestamp= now + (simtime_t) (TIME_STEP * Random());
			ScheduleNewEvent(receiver, timestamp, CELL_IN, &new_event_content, sizeof(new_event_content));
			break;
      	default:
			rootsim_error(true, "Error: unsupported event: %d\n", event_type);
			break;
	}
}

int OnGVT(unsigned int me, lp_cell *snapshot) {

 	if(snapshot->events > MAX_EVENTS)
		return true;

	return false;
}
