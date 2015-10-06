#include <ROOT-Sim.h>
#include <stdio.h>
#include <limits.h>
#include <strings.h>

#include "application.h"


//#define DEBUG


//TODO: parametrization is needed
//TODO: odd for the model????
void ProcessEvent(unsigned int me, simtime_t now, int event_type, void *event_content, int event_size, lp_cell *current_cell) {
	event_migrate new_event_content;
	initEvent(&new_event_content);
	int i;
	int receiver;
	simtime_t timestamp = 0;

	(void)event_size;

	switch(event_type) {

		case INIT: 
	
			current_cell = (lp_cell *)malloc(sizeof(lp_cell));
			
			if(current_cell == NULL){
				fprintf(stderr, "%s:%d: Unable to allocate memory!\n", __FILE__, __LINE__);
				abort();
			}
			current_cell->my_state = (state_cell *)malloc(sizeof(state_cell));

			SetState(current_cell);
			
			if(NUM_CELL_OCCUPIED > n_prc_tot){
				fprintf(stderr, "%s:%d: Require more cell than available LPs\n", __FILE__, __LINE__);
				abort();
			}
			
			initCell(current_cell, me);
							
			//first it is necessary to send this event in order the sugar could grows
			timestamp = now + (simtime_t) (TIME_STEP);
			ScheduleNewEvent(me, timestamp, UPDATE_ME, NULL, 0);
			
			//create the CELL_IN event for the first time 
			new_event_content.info_cell.id_cell = me;
			new_event_content.info_cell.direction_origin = -1; //because is the first time
			new_event_content.info_cell.occupied = 0;
			
			new_event_content.info_agent.vision = new_event_content.info_agent.metabolic_rate = new_event_content.info_agent.max_age = -1;
			new_event_content.info_agent.age = new_event_content.info_agent.wealth = -1;
			new_event_content.info_cell.newagent = RandomRange(0,1);

			timestamp = now + (simtime_t)(TIME_STEP * Random());
			
			if(new_event_content.info_cell.newagent){
				if((NUM_CELL_OCCUPIED % 2) == 0){
					//First and last cell have to be occuped 
					if(me < (NUM_CELL_OCCUPIED/2) || me >= ((n_prc_tot)-(NUM_CELL_OCCUPIED/2))) {
							// A CELL_IN event is been generate
						ScheduleNewEvent(me, timestamp, CELL_IN, &new_event_content, sizeof(event_migrate));
					}
				} else {
					if(me <= (NUM_CELL_OCCUPIED / 2) || me >= ((n_prc_tot) - (NUM_CELL_OCCUPIED / 2))){
							// A CELL_IN event is been generate
						ScheduleNewEvent(me, timestamp, CELL_IN, &new_event_content, sizeof(event_migrate));
					}
				}
			}
			break;
		case CELL_IN: {			
			current_cell->events++;
			
			event_migrate *event;
			event = (event_migrate *) event_content;
			
			bool dead_agent = 0; 
			
			//If is the first time, the cell is empty => we need a new agent in that cell
				if(event->info_cell.occupied == 0 && event->info_cell.newagent) {
				assignAgent(current_cell->cell_agent);
			}
			else { //If is a migration from a neighbour => copy all the data of the agent from the event
				//~ memcpy(current_cell->cell_agent, event_content, sizeof(agent));
				current_cell->cell_agent->vision = event->info_agent.vision; 
				current_cell->cell_agent->metabolic_rate = event->info_agent.metabolic_rate;     
				current_cell->cell_agent->max_age = event->info_agent.max_age;            
				current_cell->cell_agent->age = event->info_agent.age;                
				current_cell->cell_agent->wealth = event->info_agent.wealth;
				
				//Here we have to process the update of the cell
				i = event->info_cell.direction_origin;
				current_cell->neighbours_state[i].id = event->info_cell.id_cell;
				current_cell->neighbours_state[i].sugar_tank = event->info_cell.sugar_tank; 				
				current_cell->neighbours_state[i].occupied = event->info_cell.occupied; 				
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
				
				new_event_content.info_cell.occupied = 0;
				new_event_content.info_cell.id_cell = me;
				new_event_content.info_cell.newagent = 1;
				new_event_content.info_cell.direction_origin = directionOriginDepNeighbours(receiver);
				new_event_content.info_cell.sugar_tank = current_cell->my_state->sugar_tank;
		
				// for this event, it is not important the rest of the params of the event, maybe it will be better if we have two types of events, more clean
				new_event_content.info_agent.vision = new_event_content.info_agent.metabolic_rate = new_event_content.info_agent.max_age = -1;
				new_event_content.info_agent.age = new_event_content.info_agent.wealth = -1;

				timestamp= now + (simtime_t) (TIME_STEP);
				ScheduleNewEvent(receiver, timestamp, CELL_IN, &new_event_content, sizeof(event_migrate));			
			}
			event_cell event_update_neighbour;
			//Everytime a cell has a change (in relation with its agent), he communicates it at his neigbours
			event_update_neighbour.id_cell = me;
			event_update_neighbour.sugar_tank = current_cell->my_state->sugar_tank;
			event_update_neighbour.occupied = dead_agent; 
			event_update_neighbour.newagent = 0;
			
			for (i = 0; i < N_DIR; i++) {		
				//My direction in relation with my neighbours		
				event_update_neighbour.direction_origin = directionOriginDepNeighbours(i);
				
				timestamp = now + (simtime_t) (TIME_STEP);

				receiver = GetReceiver(TOPOLOGY_TORUS, i); //direction in ROOTSim
			
				if(receiver == -1)
					abort();
				ScheduleNewEvent(receiver, timestamp, UPDATE_NEIGHBORS, &event_update_neighbour, sizeof(event_cell));
			}

			// A CEL_OUT event is been generated
			timestamp = now + (simtime_t) (TIME_STEP * Random());
			ScheduleNewEvent(me, timestamp, CELL_OUT, NULL, 0);
		}
			break;
		case UPDATE_ME:
			//if the cell is occupied, the agent in that cell loses wealth in a metabolicRate proportion
			//this not happen the first time
			current_cell->events++;
			if(current_cell->my_state->occupied) { 
				current_cell->cell_agent->wealth-= current_cell->cell_agent->metabolic_rate;
			}

			int sugar = current_cell->my_state->sugar_tank + ALPHA;
			
			if(sugar <= current_cell->my_state->sugar_capacity) current_cell->my_state->sugar_tank = sugar;
			else  current_cell->my_state->sugar_tank = current_cell->my_state->sugar_capacity;
			
			timestamp= now + (simtime_t) (TIME_STEP);
			ScheduleNewEvent(me, timestamp, UPDATE_ME, NULL, 0);

			break;
			
		case UPDATE_NEIGHBORS:
		{
			event_cell *event;
			event = (event_cell *) event_content;
			current_cell->neighbours_state[event->direction_origin].sugar_tank = event->sugar_tank;
			current_cell->neighbours_state[event->direction_origin].occupied = event->occupied;
			break;
		}
		case CELL_OUT:
		{
			current_cell->events++;
			// Go to the neighbour who has the biggest sugarTank
			int max_sugar_cell = 0;
			event_migrate e_migrate; 
			event_cell event_update_neighbour;
			int migrated_receiver;

			receiver = -1;
			
			for(i = 0; i < N_DIR; i++) {
				//if in that cell there are sugar and it is not occupied
				if(current_cell->neighbours_state[i].sugar_tank >= max_sugar_cell && !current_cell->neighbours_state[i].occupied) {
					//The current cell eats the sugar of the new cell
					//receiver = current_cell->neighbours_state[i].id;
					receiver = i;
					max_sugar_cell = current_cell->neighbours_state[i].sugar_tank;
					break;
				}
			}

			// The 'if' condition in the loop could be not verified if all the neighbours
			// are occupied. In that case, we "wait" for some time, to see if we can migrate later
			if(receiver == -1) {
				timestamp = now + (simtime_t) (TIME_STEP * Random());
				ScheduleNewEvent(me, timestamp, CELL_OUT, NULL, 0);
				printf("Not moving\n");
				break;
			}
			
			if(receiver >= N_DIR) {
				receiver = RandomRange(0, N_DIR - 1);
			}
			
			//~ new_event_content.id_cell = me; //this is just to indicate that is not a new agent, just a migration
			//~ new_event_content.direction_origin = directionOriginDepNeighbours(receiver);
			//~ new_event_content.occupied = 1;
			//~ new_event_content.sugar_tank = current_cell->my_state->sugar_tank;
			//~ new_event_content.newagent = 0;
			//~ 
			//~ new_event_content.vision = current_cell->cell_agent->vision; 
			//~ new_event_content.metabolic_rate = current_cell->cell_agent->metabolic_rate;     
			//~ new_event_content.max_age = current_cell->cell_agent->max_age;            
			//~ new_event_content.age = current_cell->cell_agent->age;                
			//~ new_event_content.wealth = current_cell->cell_agent->wealth; 
			event_update_neighbour.id_cell = e_migrate.info_cell.id_cell = me;
			event_update_neighbour.occupied = e_migrate.info_cell.occupied = 0; //im migrating
			event_update_neighbour.sugar_tank = e_migrate.info_cell.sugar_tank = current_cell->my_state->sugar_tank;
			event_update_neighbour.newagent = e_migrate.info_cell.newagent = 0;
			e_migrate.info_cell.direction_origin = directionOriginDepNeighbours(receiver);
			
			e_migrate.info_agent.age = current_cell->cell_agent->age;
			e_migrate.info_agent.vision = current_cell->cell_agent->vision; 
			e_migrate.info_agent.metabolic_rate = current_cell->cell_agent->metabolic_rate;     
			e_migrate.info_agent.max_age = current_cell->cell_agent->max_age;            
			e_migrate.info_agent.wealth = current_cell->cell_agent->wealth;
			
			migrated_receiver = receiver; //to avoid the sending of an unnecessary event
			 
			receiver = GetReceiver(TOPOLOGY_TORUS, receiver); //direction		
					
			timestamp = now + (simtime_t) (TIME_STEP * Random());
			
			if(receiver == -1)
				abort();
			
			ScheduleNewEvent(receiver, timestamp, CELL_IN, &e_migrate, sizeof(event_migrate));
			//~ ScheduleNewEvent(receiver, timestamp, CELL_IN, current_cell->cell_agent, sizeof(agent));
			
			//I need to inform my neighbourd of my change of state, less the neighbour that received my migration
			for (i = 0; i < N_DIR; i++) {
				if(i != migrated_receiver) {
					//My direction in relation with my neighbours		
					event_update_neighbour.direction_origin = directionOriginDepNeighbours(i);
					
					timestamp = now + (simtime_t) (TIME_STEP);

					receiver = GetReceiver(TOPOLOGY_TORUS, i); //direction in ROOTSim
				
					if(receiver == -1)
						abort();
					ScheduleNewEvent(receiver, timestamp, UPDATE_NEIGHBORS, &event_update_neighbour, sizeof(event_cell));
				}		
			}
			
			break;
		}
      	default:
			fprintf(stderr, "Error: unsupported event: %d\n", event_type);
			abort();
			break;
	}
}

int OnGVT(unsigned int me, lp_cell *snapshot) {
	#ifndef DEBUG
	(void)me;
	#endif

 	if(snapshot->events > MAX_EVENTS){
		return true;
	} else {
		#ifdef DEBUG
		printf ("%d: executed %d events - state: occupied=%d, sugar tank=%d\n", me, snapshot->events, snapshot->my_state->occupied, snapshot->my_state->sugar_tank);
		#endif
	}

	return false;
}
