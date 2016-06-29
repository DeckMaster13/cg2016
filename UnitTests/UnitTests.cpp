// UnitTests.cpp : Defines the entry point for the console application.
//

#include "Source.cpp"

static void assert(bool b)
{
   if (!b) throw;
}

static void testComputeDistance()
{
   Entity e1(0, 0, 0, 0, 0, 0);
   Entity e2(0, 4, 3, 0, 0, 0);
   int distance = computeDistance(e1, e2);
   assert(distance == 5);
}

static void testComputeDistanceFromBase()
{
   Entity e1(0, 4, 3, 0, 0, 0);
   g_myBaseCoord = make_pair(0,0);
   int distance = computeDistanceFromBase(e1);
   assert(distance == 5);
}

static void testCanRelease()
{
   g_myBaseCoord = make_pair(0, 0);

   Entity e1(0, 4, 3, 0, 0, 0);
   assert(canRelease(e1) == true);

   Entity e2(0, 5000, 5000, 0, 0, 0);
   assert(canRelease(e2) == false);

   Entity e3(0, 10000, 8000, 0, 0, 0);
   assert(canRelease(e3) == false);
}

static void testSelectClosestGhost()
{
   Entity b1(0, 0, 0, 0, 0, 0);
 
   Entity g1(1, 4, 3, 0, 0, 0);
   Entity g2(2, 6, 9, 0, 0, 0);
   Entity g3(3, 10, 5, 0, 0, 0);
   Entity g4(4, 20, 5, 0, 0, 0);

   g_ghosts.clear();
   g_ghosts.push_back(g1);
   g_ghosts.push_back(g2);
   g_ghosts.push_back(g3);
   g_ghosts.push_back(g4);

   const Entity& closestGhost = selectClosestGhost(b1);
   assert(closestGhost.m_id == 1);
}

static void testCanStun()
{
   Entity b1(0, 0, 0, 0, 0, 0);

   Entity e1(1, 4, 3, 0, 0, 0);
   Entity e2(2, 100, 200, 0, 0, 0);
   Entity e3(3, 5000, 5000, 0, 0, 0);

   assert(canStun(b1, e1) == true);
   assert(canStun(b1, e2) == true);
   assert(canStun(b1, e3) == false);
}

static void testClosestEnnemy()
{
   Entity b1(0, 0, 0, 0, 0, 0);

   Entity e1(1, 20, 10, 0, 0, 0);
   Entity e2(2, 100, 200, 0, 0, 0);
   Entity e3(3, 5000, 5000, 0, 0, 0);

   g_ghosts.clear();
   g_hisBusters.push_back(e1);
   g_hisBusters.push_back(e2);
   g_hisBusters.push_back(e3);

   pair<bool, Entity> closestEnnemy = selectClosestEnnemy(b1);
   assert(closestEnnemy.first == true);
   assert(closestEnnemy.second.m_id == e1.m_id);
}

static void testClosestEnnemyWithGhost()
{
   Entity b1(0, 0, 0, 0, 0, 0);

   Entity e1(1, 20, 10, 0, 0, 0);
   Entity e2(2, 100, 200, 0, 1, 0);//carry ghost
   Entity e3(3, 5000, 5000, 0, 0, 0);

   g_ghosts.clear();
   g_hisBusters.push_back(e1);
   g_hisBusters.push_back(e2);
   g_hisBusters.push_back(e3);

   pair<bool, Entity> closestEnnemy = selectClosestEnnemyWithGhost(b1);
   assert(closestEnnemy.first == true);
   assert(closestEnnemy.second.m_id == e2.m_id);
}

int main()
{
   testComputeDistance();
   testSelectClosestGhost();
   testComputeDistanceFromBase();
   testCanRelease();
   testCanStun();
   testClosestEnnemy();
   testClosestEnnemyWithGhost();
	return 0;
}

