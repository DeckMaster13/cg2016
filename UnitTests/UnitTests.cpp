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

static void testComputeNewPositionIfMoveToward()
{
   Entity b1(1, 0, 0, 0, 0, 0);

   pair<int, int> destination1(1500, 0);
   assert(computeNewPositionIfMoveToward(b1, destination1) == make_pair(800, 0));

   pair<int, int> destination2(0, 1500);
   assert(computeNewPositionIfMoveToward(b1, destination2) == make_pair(0, 800));

   pair<int, int> destination3(400, 400);
   assert(computeNewPositionIfMoveToward(b1, destination3) == make_pair(565, 565));

   pair<int, int> destination4(300, 400);
   assert(computeNewPositionIfMoveToward(b1, destination4).first < computeNewPositionIfMoveToward(b1, destination4).second);

   Entity b2(1, 9000, 9000, 0, 0, 0);

   pair<int, int> destination5(0, 0);
   assert(computeNewPositionIfMoveToward(b2, destination5) == make_pair(8434, 8434));
}

static void testCanCatchBeforeRelease()
{
   Entity b1(0, 0, 0, 0, 0, 0);
   Entity e1(1, 400, 400, 0, 0, 0);
   Entity e2(2, 800, 800, 0, 0, 0);
   Entity e3(2, 3000, 3000, 0, 0, 0);
   g_ennemyBasePeripheryCoord.first = 14350;
   g_ennemyBasePeripheryCoord.second = 7350;
   g_ennemyBaseCoord.first = 16000;
   g_ennemyBaseCoord.second= 9000;
   assert(canCatchBeforeRelease(b1, e1) > 0);
   assert(canCatchBeforeRelease(b1, e2) > 0);
   assert(canCatchBeforeRelease(b1, e3) == 0);

   //real buggy test
   Entity b2(42, 7577, 2202, 0, 0, 0);
   Entity e4(43, 8833, 4052, 0, 0, 0);
   assert(canCatchBeforeRelease(b2, e4) == 0);

   g_ennemyBasePeripheryCoord.first = 1650;
   g_ennemyBasePeripheryCoord.second = 1650;
   g_ennemyBaseCoord.first = 0;
   g_ennemyBaseCoord.second = 0;
   Entity b3(43, 6400, 3275, 0, 0, 0);
   Entity e5(44, 14680, 6200, 0, 0, 0);
   assert(canCatchBeforeRelease(b3, e5) > 0);
}

static void testPositionValid()
{
   pair<int, int> pos(16000,9000);
   assert(isPositionValid(pos) == true);

   pos = make_pair(0,0);
   assert(isPositionValid(pos) == true);

   pos = make_pair(9000, 4000);
   assert(isPositionValid(pos) == true);

   pos = make_pair(200, 6584);
   assert(isPositionValid(pos) == true);

   pos = make_pair(10000, 15000);
   assert(isPositionValid(pos) == false);

   pos = make_pair(20000, 2000);
   assert(isPositionValid(pos) == false);
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
   testComputeNewPositionIfMoveToward();
   testCanCatchBeforeRelease();
	return 0;
}

