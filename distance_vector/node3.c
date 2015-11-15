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
} dt3;

struct route_table 
{
  bool route[4][4];
} rt3;

struct path_value
{
  int value;
  int node_id;
  bool route[4];
} pv3;

struct neighbors
{
  bool nodes[4];
} nb3;

struct link_costs
{
  int costs[4];
} ln3;


void sendPacketsNode3()
{
  sendPackets(&dt3, &rt3, &nb3, 3);
}

bool updateCostsNode3(int sourceid, int mincosts[4])
{
  return updateCosts(&dt3, sourceid, mincosts);
}

bool calculatePathsNode3()
{
  return calculatePaths(&dt3, &ln3, &rt3, &nb3, 3);
}

void rtinit3() 
{
  // Message
  printf("rtinit3 called at %f\r\n", clocktime);

  // Initialize the distance table
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      dt3.costs[i][j] = 999;

  // Set the default values
  dt3.costs[0][3] = ln3.costs[0] = 7;
  dt3.costs[1][3] = ln3.costs[1] = 999;
  dt3.costs[2][3] = ln3.costs[2] = 2;
  dt3.costs[3][3] = ln3.costs[3] = 0;

  // Set the neighboring elements
  nb3.nodes[0] = 1;
  nb3.nodes[1] = 0;
  nb3.nodes[2] = 1;
  nb3.nodes[3] = 0;

  // Send packets to neighbors
  sendPacketsNode3();
}


void rtupdate3(rcvdpkt)
  struct rtpkt *rcvdpkt;
  
{
  // Message
  printf("rtupdate3 called by node #%d at %f\r\n", rcvdpkt->sourceid, clocktime);

  /* Perform Distance Vector algorithm */
  bool b_update = updateCostsNode3(rcvdpkt->sourceid, rcvdpkt->mincost);

  // Now calculate paths
  bool b_costs_update = calculatePathsNode3();

  // Report results
  printf("rtupdate3: distance table was %supdated.\r\n", (b_update || b_costs_update) ? "" : "not ");  
  if (b_update || b_costs_update)
    printdt3(&dt3);
}


printdt3(dtptr)
  struct distance_table *dtptr;
  
{
  printf("             via     \n");
  printf("   D3 |    0     2 \n");
  printf("  ----|-----------\n");
  printf("     0|  %3d   %3d\n",dtptr->costs[0][0], dtptr->costs[0][2]);
  printf("dest 1|  %3d   %3d\n",dtptr->costs[1][0], dtptr->costs[1][2]);
  printf("     2|  %3d   %3d\n",dtptr->costs[2][0], dtptr->costs[2][2]);

}
