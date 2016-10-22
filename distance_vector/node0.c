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
} dt0;

struct route_table 
{
  bool route[4][4];
} rt0;

struct path_value
{
  int value;
  int node_id;
  bool route[4];
} pv0;

struct neighbors
{
  bool nodes[4];
} nb0;

struct link_costs
{
  int costs[4];
} ln0;


bool isNodeInPath(struct route_table * rt, int target_id, int hop_id)
{
  return rt->route[target_id][hop_id];
}

void setNodeInPath(struct route_table * rt, int target_id, int hop_id, bool b_in_path)
{
  rt->route[target_id][hop_id] = b_in_path;
}

void setNewPathValue(struct distance_table * dt, struct route_table * rt, int node_id, struct path_value pv)
{
  // Set the new value
  dt->costs[pv.node_id][node_id] = pv.value;

  // Reset the current path
  for (int i = 0; i < 4; i++)
    setNodeInPath(rt, pv.node_id, i, pv.route[i]);
}

void sendPackets(struct distance_table * dt, struct route_table * rt, struct neighbors * nb, int node_id)
{
  // Submit the costs to neighbors
  for (int dest_id = 0; dest_id < 4; dest_id++)
  {
    // Check if the nodes are connected
    if (!nb->nodes[dest_id] || node_id == dest_id) continue;

    // Format the packet
    struct rtpkt pkt2sen;
    pkt2sen.sourceid = node_id;
    pkt2sen.destid   = dest_id;

    // Provide the costs - 999 if the destination is en route to j
    for (int j = 0; j < 4; j++)
    {
      pkt2sen.mincost[j] = (isNodeInPath(rt, j, dest_id) ? 999 : dt->costs[j][node_id]);
    }

    // Send the packet
    tolayer2(pkt2sen);
  }
}

struct path_value distanceVector(struct distance_table * dt, struct link_costs * ln, 
  int node_id, int target_id, int path_cost, bool visited[4])
{
  // The minimum value path
  struct path_value mpv;
  mpv.value = 999;

  for (int i = 0; i < 4; i++)
  {
    // Skip visited nodes and non-traversable nodes
    if (visited[i] || ln->costs[i] >= 999 || dt->costs[target_id][i] >= 999) continue;
    visited[i] = true;

    // Get the next path & value
    struct path_value pv;
    pv.node_id = target_id;
    pv.value = ln->costs[i] + dt->costs[target_id][i];

    // Keep track of the route for poisoned reverse
    for (int j = 0; j < 4; j++) 
      pv.route[j] = visited[j];
    pv.route[target_id] = 0;

    // Keep track of the best path
    if (pv.value < mpv.value)
    {
      mpv = pv;
    }

    visited[i] = false;
  }

  // Return the minimal cost
  return mpv;
}

bool updateCosts(struct distance_table * dt, int sourceid, int mincosts[4])
{
  bool b_update = false;

  // First fill in newly reported values
  for (int i = 0; i < 4; i++)
  {
    // Determine if we're updating the table
    b_update |= dt->costs[i][sourceid] != mincosts[i];

    // Copy the cost either way
    dt->costs[i][sourceid] = mincosts[i];
  }

  return b_update;
}

bool calculatePaths(struct distance_table * dt, struct link_costs * ln, 
  struct route_table * rt, struct neighbors * nb, int node_id)
{
  bool b_update = false;

  // Now calculate paths
  for (int i = 0; i < 4; i++)
  {
    // Obviously don't calculate the path to itself
    if (node_id == i) continue;

    // Determine the best path using the distance vector algo
    bool visited[4] = {(node_id==0), (node_id==1), (node_id==2), (node_id==3)};
    struct path_value pv = distanceVector(dt, ln, node_id, i, 0, &visited);

    // If the path changes, notify - this could be a different value, or different route
    if (pv.value != dt->costs[pv.node_id][node_id] ||
        rt->route[i][0] != pv.route[0] || rt->route[i][1] != pv.route[1] || 
        rt->route[i][2] != pv.route[2] || rt->route[i][3] != pv.route[3])
    {
      // Set the new path value
      b_update = true;
      setNewPathValue(dt, rt, node_id, pv);

      // Console Notifications
      printf("rtupdate%d: cost to node #%d updated to %d\r\n", node_id, pv.node_id, pv.value);
    }
  }

  if (b_update)
  {
    printf("rtupdate%d: sending out packets to neighbors with new values: [%d,%d,%d,%d]\r\n", 
      node_id, dt->costs[0][node_id], dt->costs[1][node_id], dt->costs[2][node_id], dt->costs[3][node_id]);

    // Deliver packets to other nodes
    sendPackets(dt, rt, nb, node_id);
  }

  // Report results
  return b_update;
}

// For debugging purposes
void printdt(struct distance_table * dtptr, int node_id)  
{
  printf("                   via        \n");
  printf("   D%d |    0     1     2    3 \n", node_id);
  printf("  ----|-----------------------\n");
  printf("     0|  %3d   %3d   %3d   %3d\n",dtptr->costs[0][0], dtptr->costs[0][1], dtptr->costs[0][2],dtptr->costs[0][3]);
  printf("dest 1|  %3d   %3d   %3d   %3d\n",dtptr->costs[1][0], dtptr->costs[1][1], dtptr->costs[1][2],dtptr->costs[1][3]);
  printf("     2|  %3d   %3d   %3d   %3d\n",dtptr->costs[2][0], dtptr->costs[2][1], dtptr->costs[2][2],dtptr->costs[2][3]);
  printf("     3|  %3d   %3d   %3d   %3d\n",dtptr->costs[3][0], dtptr->costs[3][1], dtptr->costs[3][2],dtptr->costs[3][3]);
}


/*
  Node specific
*/
void sendPacketsNode0()
{
  sendPackets(&dt0, &rt0, &nb0, 0);
}

bool updateCostsNode0(int sourceid, int mincosts[4])
{
  return updateCosts(&dt0, sourceid, mincosts);
}

bool calculatePathsNode0()
{
  return calculatePaths(&dt0, &ln0, &rt0, &nb0, 0);
}

void rtinit0() 
{
  // Message
  printf("rtinit0 called at %f\r\n", clocktime);

  // Initialize the distance table
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      dt0.costs[i][j] = 999;

  // Set the default values
  dt0.costs[0][0] = ln0.costs[0] = 0;
  dt0.costs[1][0] = ln0.costs[1] = 1;
  dt0.costs[2][0] = ln0.costs[2] = 3;
  dt0.costs[3][0] = ln0.costs[3] = 7;

  // Set the neighboring elements
  nb0.nodes[0] = 0;
  nb0.nodes[1] = 1;
  nb0.nodes[2] = 1;
  nb0.nodes[3] = 1;

  // Send packets to neighbors
  sendPacketsNode0();
}

void rtupdate0(rcvdpkt)
  struct rtpkt *rcvdpkt;
{
  // Message
  printf("rtupdate0 called by node #%d at %f\r\n", rcvdpkt->sourceid, clocktime);

  /* Perform Distance Vector algorithm */
  bool b_update = updateCostsNode0(rcvdpkt->sourceid, rcvdpkt->mincost);

  // Now calculate paths
  bool b_costs_update = calculatePathsNode0();

  // Report results
  printf("rtupdate0: distance table was %supdated.\r\n", (b_update || b_costs_update) ? "" : "not ");
  if (b_update || b_costs_update)
    printdt0(&dt0);
}


printdt0(dtptr)
  struct distance_table *dtptr;
  
{
  printf("                via     \n");
  printf("   D0 |    1     2    3 \n");
  printf("  ----|-----------------\n");
  printf("     1|  %3d   %3d   %3d\n",dtptr->costs[1][1],
	 dtptr->costs[1][2],dtptr->costs[1][3]);
  printf("dest 2|  %3d   %3d   %3d\n",dtptr->costs[2][1],
	 dtptr->costs[2][2],dtptr->costs[2][3]);
  printf("     3|  %3d   %3d   %3d\n",dtptr->costs[3][1],
	 dtptr->costs[3][2],dtptr->costs[3][3]);
}

/* change the value of the LINKCHANGE constant definition in prog3.c from 0 to 1 */
linkhandler0(linkid, newcost)   
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
  ln0.costs[linkid] = newcost;

  // Set the new path value
  setNewPathValue(&dt0, &rt0, 0, pv);

  // Debugging
  printf("linkhandler0: new values: [%d,%d,%d,%d]\r\n", 
        dt0.costs[0][0], dt0.costs[1][0], dt0.costs[2][0], dt0.costs[3][0]);

  // Now calculate paths
  calculatePathsNode0();

  // Report results
  printf("linkhandler0: distance table was updated - cost to %d is now %d.\r\n", linkid, newcost);
  printdt0(&dt0);
}
