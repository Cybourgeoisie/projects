#include <stdio.h>
#include <stdbool.h>

extern struct rtpkt {
  int sourceid;       /* id of sending router sending this pkt */
  int destid;         /* id of router to which pkt being sent 
                         (must be an immediate neighbor) */
  int mincost[4];    /* min cost to node 0 ... 3 */
  };


extern int TRACE;
extern int YES;
extern int NO;
extern float clocktime;

struct distance_table 
{
  int costs[4][4];
} dt1;

struct route_table 
{
  bool route[4][4];
} rt1;

struct path_value
{
  int value;
  int node_id;
  bool route[4];
} pv1;

struct neighbors
{
  bool nodes[4];
} nb1;

struct link_costs
{
  int costs[4];
} ln1;


void sendPacketsNode1()
{
  sendPackets(&dt1, &rt1, &nb1, 1);
}

bool updateCostsNode1(int sourceid, int mincosts[4])
{
  return updateCosts(&dt1, sourceid, mincosts);
}

bool calculatePathsNode1()
{
  return calculatePaths(&dt1, &ln1, &rt1, &nb1, 1);
}

rtinit1()
{
  // Message
  printf("rtinit1 called at %f\r\n", clocktime);

  // Initialize the distance table
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      dt1.costs[i][j] = 999;

  // Set the default values
  dt1.costs[0][1] = ln1.costs[0] = 1;
  dt1.costs[1][1] = ln1.costs[1] = 0;
  dt1.costs[2][1] = ln1.costs[2] = 1;
  dt1.costs[3][1] = ln1.costs[3] = 999;

  // Set the neighboring elements
  nb1.nodes[0] = 1;
  nb1.nodes[1] = 0;
  nb1.nodes[2] = 1;
  nb1.nodes[3] = 0;

  // Send packets to neighbors
  sendPacketsNode1();
}

rtupdate1(rcvdpkt)
  struct rtpkt *rcvdpkt;
  
{
  // Message
  printf("rtupdate1 called by node #%d at %f\r\n", rcvdpkt->sourceid, clocktime);

  /* Perform Distance Vector algorithm */
  bool b_update = updateCostsNode1(rcvdpkt->sourceid, rcvdpkt->mincost);

  // Now calculate paths
  bool b_costs_update = calculatePathsNode1();

  // Report results
  printf("rtupdate1: distance table was %supdated.\r\n", (b_update || b_costs_update) ? "" : "not ");  
  if (b_update || b_costs_update)
    printdt1(&dt1);
}

printdt1(dtptr)
  struct distance_table *dtptr;
  
{
  printf("             via   \n");
  printf("   D1 |    0     2 \n");
  printf("  ----|-----------\n");
  printf("     0|  %3d   %3d\n",dtptr->costs[0][0], dtptr->costs[0][2]);
  printf("dest 2|  %3d   %3d\n",dtptr->costs[2][0], dtptr->costs[2][2]);
  printf("     3|  %3d   %3d\n",dtptr->costs[3][0], dtptr->costs[3][2]);

}

/* change the value of the LINKCHANGE constant definition in prog3.c from 0 to 1 */
linkhandler1(linkid, newcost)   
int linkid, newcost;
{
  // Prepare the new path value packet
  struct path_value pv;
  pv.value   = newcost;
  pv.node_id = linkid;
  pv.route[0] = 0;
  pv.route[1] = 0;
  pv.route[2] = 0;
  pv.route[3] = 0;

  // Update the link costs
  ln1.costs[linkid] = newcost;

  // Set the new path value
  setNewPathValue(&dt1, &rt1, 1, pv);

  // Debugging
  printf("linkhandler1: new values: [%d,%d,%d,%d]\r\n", 
        dt1.costs[0][1], dt1.costs[1][1], dt1.costs[2][1], dt1.costs[3][1]);

  // Now calculate paths
  calculatePathsNode1();

  // Report results
  printf("linkhandler1: distance table was updated - cost to %d is now %d.\r\n", linkid, newcost);
  printdt1(&dt1);
}
