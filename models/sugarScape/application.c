#include <ROOT-Sim.h>
#include <stdio.h>
#include <limits.h>

#include "application.h"

//TODO: how can i used a distribution between [1,6] for ex
//TODO: parametrization is needed
//TODO: odd for the model????
void ProcessEvent(int me, simtime_t now, int event_type, void *event_content, int event_size, lp_cell *current_cell) {

	event_content_type new_event_content;
	
	initEvent(&new_event_content);
	
	int i;
	int receiver;
	
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
					printf("init model %d...\n", me);
				}
			} else {
				if(me <= (NUM_CELL_OCCUPIED / 2) || me >= ((n_prc_tot) - (NUM_CELL_OCCUPIED / 2))){
						// A CELL_IN event is been generate
					ScheduleNewEvent(me, timestamp, CELL_IN, &new_event_content, sizeof(new_event_content));
					printf("init model 2 - %d...\n", me);
				}
			}
			break;
		case CELL_IN: {
			current_cell->events++;
			bool dead_agent = 0; 
			
			printf("cell in from %d..\n", me);
			//If is the first time, the cell is empty => we need a new agent in that cell
			if(event_content->occupied == 0 && event_content->newagent) { 
				assignAgent(current_cell->cell_agent);
			}
			else { //If is a migration from a neighbour => copy all the data of the agent from the event
				//~ memcpy(current_cell->cell_agent, event_content, sizeof(agent));
				current_cell->cell_agent->vision = event_content->vision; 
				current_cell->cell_agent->metabolic_rate = event_content->metabolic_rate;     
				current_cell->cell_agent->max_age = event_content->max_age;            
				current_cell->cell_agent->age = event_content->age;                
				current_cell->cell_agent->wealth = event_content->wealth;             
			}
			
			//The normal behaviour of an agent in a cell is to eat the sugar of it (for the moment we just have one agent per cell)
			current_cell->cell_agent->wealth+= current_cell->my_state->sugar_tank;
			current_cell->my_state->sugar_tank = 0;
			
			//check if an agent is dead => it is necessary to grown a new agent in a random cell
			if(current_cell->cell_agent->wealth <= 0) { 
				dead_agent = 1;
				deadAgent(current_cell->cell_agent);
				//find a random cell 
				receiver = FindReceiver(TOPOLOGY_MESH);
				
				new_event_content.occupied = 0;
				new_event_content.id_cell = me;
				new_event_content.newagent = 1;
				//new_event_content.direction_origin = ???;
				new_event_content.sugar_tank = current_cell->my_state->sugar_tank;
		
			//for this event, it is not important the rest of the params of the event, maybe it will be better if we have two types of events, more clean
				new_event_content.vision = new_event_content.metabolic_rate = new_event_content.max_age = -1;
				new_event_content.age = new_event_content.wealth = -1;
				
				timestamp= now + (simtime_t) (TIME_STEP);
				ScheduleNewEvent(receiver, timestamp, CELL_IN, &new_event_content, sizeof(new_event_content));			
			}
			
			//Everytime a cell has a change (in relation with its agent), he communicates it at his neigbours
			new_event_content.id_cell = me;
            new_event_content.sugar_tank = current_cell->my_state->sugar_tank;
			new_event_content.occupied = dead_agent?dead_agent:1; 
			new_event_content.newagent = 0;
			//for this event, it is not important the rest of the params of the event, maybe it will be better if we have two types of events, more clean
			new_event_content.vision = new_event_content.metabolic_rate = new_event_content.max_age = new_event_content.age = -1;
			new_event_content.wealth = -1; 
			
			for (i = 0; i < N_DIR; i++) {
				
				switch (i){
					case DIRECTION_N: //updating northern neighbour, i am coming from south
						new_event_content.direction_origin = DIRECTION_S;
					 break;
					case DIRECTION_S: //updating southern neighbour, i am coming from north
						new_event_content.direction_origin = DIRECTION_N;
					break;
					case DIRECTION_E: 
						new_event_content.direction_origin = DIRECTION_W;
					break;
					case DIRECTION_W: 
						new_event_content.direction_origin = DIRECTION_E;
					break;
				}
				timestamp = now + (simtime_t) (TIME_STEP);

				receiver = GetReceiver(TOPOLOGY_TORUS, i); //direction in ROOTSim
			
				if(receiver == -1)
					abort();
				ScheduleNewEvent(receiver, timestamp, UPDATE_NEIGHBORS, &new_event_content, sizeof(new_event_content));
				printf("update neighbors from %d....\n", me);
			}

			// A CEL_OUT event is been generated
			timestamp = now + (simtime_t) (TIME_STEP * Random());
			ScheduleNewEvent(me, timestamp, CELL_OUT, NULL, 0);
		}
			break;
		case UPDATE_ME:
			//if the cell is occupied, the agent in that cell loses wealth in a metabolicRate proportion
			//this not happen the first time
			if(current_cell->my_state->occupied) { 
				current_cell->cell_agent->wealth-= current_cell->cell_agent->metabolic_rate;
			}

			int sugar = current_cell->my_state->sugar_tank + ALPHA;
			
			if(sugar <= current_cell->my_state->sugar_capacity) current_cell->my_state->sugar_tank = sugar;
			else  current_cell->my_state->sugar_tank = current_cell->my_state->sugar_capacity;
			
			timestamp= now + (simtime_t) (TIME_STEP);
			printf("update me from %d...\n", me);
			ScheduleNewEvent(me, timestamp, UPDATE_ME, NULL, 0);

			break;
			
		case UPDATE_NEIGHBORS:
printf("inside before update neighbors from %d....\n", me);
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
					//receiver = current_cell->neighbours_state[i].id;
					receiver = i;
					max_sugar_cell = current_cell->neighbours_state[i].sugar_tank;
					break;
				}
			}
			
			if(receiver >= N_DIR) {
				receiver = RandomRange(0, N_DIR - 1);
			}
			
			new_event_content.id_cell = me; //this is just to indicate that is not a new agent, just a migration
			new_event_content.direction_origin = -1; // TODO? Who is my direction
			new_event_content.occupied = 1;
			new_event_content.sugar_tank = current_cell->my_state->sugar_tank;
			new_event_content.newagent = 0;
			
			new_event_content.vision = current_cell->cell_agent->vision; 
			new_event_content.metabolic_rate = current_cell->cell_agent->metabolic_rate;     
			new_event_content.max_age = current_cell->cell_agent->max_age;            
			new_event_content.age = current_cell->cell_agent->age;                
			new_event_content.wealth = current_cell->cell_agent->wealth; 
					
			receiver = GetReceiver(TOPOLOGY_TORUS, i); //direction		
					
			timestamp= now + (simtime_t) (TIME_STEP * Random());
				if(receiver == -1)
					abort();
			ScheduleNewEvent(receiver, timestamp, CELL_IN, &new_event_content, sizeof(new_event_content));
			//~ ScheduleNewEvent(receiver, timestamp, CELL_IN, current_cell->cell_agent, sizeof(agent));
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
