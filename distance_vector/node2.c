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
} dt2;

struct route_table 
{
  bool route[4][4];
} rt2;

struct path_value
{
  int value;
  int node_id;
  bool route[4];
} pv2;

struct neighbors
{
  bool nodes[4];
} nb2;

struct link_costs
{
  int costs[4];
} ln2;


void sendPacketsNode2()
{
  sendPackets(&dt2, &rt2, &nb2, 2);
}

bool updateCostsNode2(int sourceid, int mincosts[4])
{
  return updateCosts(&dt2, sourceid, mincosts);
}

bool calculatePathsNode2()
{
  return calculatePaths(&dt2, &ln2, &rt2, &nb2, 2);
}

void rtinit2() 
{
  // Message
  printf("rtinit2 called at %f\r\n", clocktime);

  // Initialize the distance table
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      dt2.costs[i][j] = 999;

  // Set the default values
  dt2.costs[0][2] = ln2.costs[0] = 3;
  dt2.costs[1][2] = ln2.costs[1] = 1;
  dt2.costs[2][2] = ln2.costs[2] = 0;
  dt2.costs[3][2] = ln2.costs[3] = 2;

  // Set the neighboring elements
  nb2.nodes[0] = 1;
  nb2.nodes[1] = 1;
  nb2.nodes[2] = 0;
  nb2.nodes[3] = 1;

  // Send packets to neighbors
  sendPacketsNode2();
}


void rtupdate2(rcvdpkt)
  struct rtpkt *rcvdpkt;
  
{
  // Message
  printf("rtupdate2 called by node #%d at %f\r\n", rcvdpkt->sourceid, clocktime);

  /* Perform Distance Vector algorithm */
  bool b_update = updateCostsNode2(rcvdpkt->sourceid, rcvdpkt->mincost);

  // Now calculate paths
  bool b_costs_update = calculatePathsNode2();

  // Report results
  printf("rtupdate2: distance table was %supdated.\r\n", (b_update || b_costs_update) ? "" : "not ");  
  if (b_update || b_costs_update)
    printdt2(&dt2);
}


printdt2(dtptr)
  struct distance_table *dtptr;
  
{
  printf("                via     \n");
  printf("   D2 |    0     1    3 \n");
  printf("  ----|-----------------\n");
  printf("     0|  %3d   %3d   %3d\n",dtptr->costs[0][0],
	 dtptr->costs[0][1],dtptr->costs[0][3]);
  printf("dest 1|  %3d   %3d   %3d\n",dtptr->costs[1][0],
	 dtptr->costs[1][1],dtptr->costs[1][3]);
  printf("     3|  %3d   %3d   %3d\n",dtptr->costs[3][0],
	 dtptr->costs[3][1],dtptr->costs[3][3]);
}
