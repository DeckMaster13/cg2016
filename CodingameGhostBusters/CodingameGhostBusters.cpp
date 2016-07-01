#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <utility>
#include <array>
#include <map>

using namespace std;

class Entity
{
public:
   Entity() = default;
   Entity(int id, int x, int y, int type, int state, int value)
      : m_id(id)
      , m_x(x)
      , m_y(y)
      , m_type(type)
      , m_state(state)
      , m_value(value)
      , m_reloadTime(0)
      , m_rank(-1)
   {}
   ~Entity() = default;
   Entity(const Entity& e)
      : m_id(e.m_id)
      , m_x(e.m_x)
      , m_y(e.m_y)
      , m_type(e.m_type)
      , m_state(e.m_state)
      , m_value(e.m_value)
      , m_reloadTime(e.m_reloadTime)
      , m_rank(e.m_rank)
   {}
public:
   int m_id{ -1 }; // buster id or ghost id
   int m_x;
   int m_y; // position of this buster / ghost
   int m_type; // the team id if it is a buster, -1 if it is a ghost.
   int m_state; // For busters: 0=idle, 1=carrying a ghost.
   int m_value; // For busters: Ghost id being carried. For ghosts: number of busters attempting to trap this ghost.
   int m_reloadTime; // For busters: Stun reload time.
   int m_rank; // For busters: Play order.
};

class ExplorableTile
{
public:
   ExplorableTile() = default;
   ExplorableTile(int x, int y)
      : m_x(x)
      , m_y(y)
      , m_explored(false)
   {}
   ~ExplorableTile() = default;
   ExplorableTile(const ExplorableTile&) = default;

   bool operator==(const ExplorableTile& t)
   {
      return m_x == t.m_x && m_y == t.m_y;
   }
public:
   int m_x;
   int m_y;
   bool m_explored;
};

//OVERALL SETTINGS
static int                    g_bustersPerPlayer; // the amount of busters you control
static int                    g_ghostCount; // the amount of ghosts on the map
static int                    g_myTeamId; // if this is 0, your base is on the top left of the map, if it is one, on the bottom right
static pair<int, int>         g_myBaseCoord; // coord of the base
static pair<int, int>         g_ennemyBasePeripheryCoord;
static pair<int, int>         g_ennemyBaseCoord;

//CURRENT TURN VALUES
static int                    g_entitiesCount; // the number of busters and ghosts visible to you
static vector<Entity>         g_entities; // a vector to store the entities
static vector<Entity>         g_ghosts;
static vector<Entity>         g_myBusters;
static vector<Entity>         g_hisBusters;
static vector<int>            g_idsOfCapturedGhosts;
static vector<ExplorableTile> g_explorableTiles(4 * 2);
static bool                   g_lastResortExploration{false};
static vector<ExplorableTile> g_lastResortExplorableTiles(6);
static vector<int>            g_stunReloadTimes;
static vector<Entity>         g_ennemiesWithGhosts;
static map<int, int>          g_assignedEnnemies;
static map<int, pair<int, int>>g_assignedEnnemiesTargetPos;
static map<int, Entity>       g_visibleEntities;

static void printEnnemiesWithGhost()
{
   for (auto& v : g_ennemiesWithGhosts)
      cerr << "EnnemyWithGhost: " << v.m_id << endl;
}

static void printAssignedEnnemies()
{
   for (auto& v : g_assignedEnnemies)
      cerr << "AssignedEnnemy: " << v.first << " > " << v.second << endl;
}

static void print(const Entity& e, bool enabled)
{
   if (!enabled) return;

   cerr << " -- ENTITY -- " << endl;
   cerr << "id: " << e.m_id << endl;
   cerr << "x: " << e.m_x << endl;
   cerr << "y: " << e.m_y << endl;
   cerr << "type: " << e.m_type << endl;
   cerr << "state: " << e.m_state << endl;
   cerr << "value: " << e.m_value << endl;
}

static int computeDistanceFromBase(const Entity& e1)
{
   return static_cast<int>(sqrt(pow(e1.m_x - g_myBaseCoord.first, 2) + pow(e1.m_y - g_myBaseCoord.second, 2)));
}

static int computeDistance(const Entity& e1, const Entity& e2)
{
   return static_cast<int>(sqrt(pow(e1.m_x - e2.m_x, 2) + pow(e1.m_y - e2.m_y, 2)));
}

static int computeDistance(const Entity& e1, int x, int y)
{
   return static_cast<int>(sqrt(pow(e1.m_x - x, 2) + pow(e1.m_y - y, 2)));
}

static int computeDistance(pair<int, int> p1, pair<int, int> p2)
{
   return static_cast<int>(sqrt(pow(p1.first - p2.first, 2) + pow(p1.second - p2.second, 2)));
}

static bool hasGhost(const Entity& buster)
{
   return buster.m_state == 1;
}

static bool isPositionValid(pair<int, int> coord)
{
   return 0 <= coord.first && coord.first <= 16000 && 0 <= coord.second && coord.second <= 9000;
}

static bool isStun(const Entity& buster)
{
   return buster.m_state == 2;
}

static bool isBusting(const Entity& buster)
{
   return buster.m_state == 3;
}

static bool isGhostCaptured(const Entity& ghost)
{
   for (auto id : g_idsOfCapturedGhosts)
   {
      if (id == ghost.m_id) return true;
   }
   return false;
}

static bool canSeeGhost()
{
   return g_ghosts.size() != 0;
}

static bool canStun(const Entity& buster, const Entity& ennemy)
{
   return !isStun(ennemy) && buster.m_reloadTime <= 0 && computeDistance(buster, ennemy) <= 1760;//optim ? do not stun already stun guy //optim: do not stun capturing guy
}

static bool canBust(const Entity& buster, const Entity& ghost)
{
   int distance = computeDistance(buster, ghost);
   return 900 <= distance && distance <= 1760;
}

static bool canRelease(const Entity& buster)
{
   int distance = computeDistanceFromBase(buster);
   return distance <= 1600;
}

static bool canEnnemyRelease(const Entity& ennemy)
{
   int distance = computeDistance(ennemy, g_ennemyBaseCoord.first, g_ennemyBaseCoord.second);
   return distance <= 1600;
}

static int sign(int expr)
{
   return  expr != 0 ? expr / abs(expr) : 0;
}

static pair<int, int> computeNewPositionIfMoveToward(const Entity& entity, pair<int, int> destination)
{
   if (destination.second - entity.m_y == 0 && destination.first - entity.m_x == 0)
   {
      return make_pair(entity.m_x, entity.m_y);
   }
   else if (destination.second - entity.m_y == 0)
   {
      return make_pair(entity.m_x + 800 * (destination.first - entity.m_x) / abs(destination.first - entity.m_x), entity.m_y);
   }

   int r = (destination.first - entity.m_x) / (destination.second - entity.m_y);
   int x = entity.m_x + sign(destination.first - entity.m_x) * 800 * r / (sqrt(1 + pow(r, 2)));
   int y = entity.m_y + sign(destination.second - entity.m_y) * 800 * 1 / (sqrt(1 + pow(r, 2)));
   return make_pair(x, y);
}

//return the distance to cross to intercept, 0 means not possible
//naive: intercept only close to base
static int canCatchBeforeRelease(const Entity& buster, const Entity& ennemy, pair<int, int>& targetPos)
{
   std::vector< pair<int, int> > ennemyPositions;
   Entity copiedEnnemy(ennemy);
   while (!canEnnemyRelease(copiedEnnemy))
   {
      pair<int, int> ennemyPosition = computeNewPositionIfMoveToward(copiedEnnemy, g_ennemyBaseCoord);
      copiedEnnemy.m_x = ennemyPosition.first;
      copiedEnnemy.m_y = ennemyPosition.second;

      if (!isPositionValid(ennemyPosition))
      {
         break;
      }
      ennemyPositions.push_back(ennemyPosition);
   }

   if (canStun(buster, ennemy))
   {
      return 1;//...
   }

   int turn = 0;
   int reloadTime = buster.m_reloadTime;
   for (auto pos : ennemyPositions)
   {
      int distance = computeDistance(buster, pos.first, pos.second);
      int range = turn * 800 + 1600;
      if (reloadTime <= 0 && distance < range)//stun distance
      {
         targetPos = pos;
         return distance;
      }
      reloadTime--;
      turn++;
   }

   return 0;
}

static void fillTrackingVector()
{
   for (auto ennemy : g_ennemiesWithGhosts)
   {
      int minDistance = 32000;
      Entity minBuster;
      pair<int, int> minTargetPos;
      bool found = false;

      for (auto buster : g_myBusters)
      {
         if (!isStun(buster))
         {
            cerr << "BUSTER COULD TRACK" << endl;
            pair<int, int> targetPos;
            int currentDistance = canCatchBeforeRelease(buster, ennemy, targetPos);
            if (currentDistance != 0 && currentDistance < minDistance)
            {
               cerr << "BUSTER CAN TRACK, ENNEMY: " << ennemy.m_id << endl;
               minDistance = currentDistance;
               minBuster = buster;
               minTargetPos = targetPos;
               found = true;
            }
            else
            {
               cerr << "BUSTER CANNOT TRACK, DISTANCE : " << currentDistance << endl;
            }
         }
         else
         {
            cerr << "BUSTER IS STUNNED CANNOT TRACK" << endl;
         }
      }
      if (found)
      {
         cerr << "BUSTER ASSIGNED TO TRACK, ENNEMY: " << ennemy.m_id << endl;
         g_assignedEnnemies[minBuster.m_rank] = ennemy.m_id;
         g_assignedEnnemiesTargetPos[minBuster.m_rank] = minTargetPos;
      }
   }
}

//SHOULD BE USELESS
/*
static void updateTrackingVector()
{
for (auto& assignedEnnemy : g_assignedEnnemies)
{
if (!hasGhost(assignedEnnemy.second))
{
assignedEnnemy.second.m_id = 0;
}
}
}
*/

static void readGameSettings()
{
   cin >> g_bustersPerPlayer; cin.ignore();
   cin >> g_ghostCount; cin.ignore();
   cin >> g_myTeamId; cin.ignore();
   if (g_myTeamId == 1)
   {
      g_myBaseCoord.first = 16000;
      g_myBaseCoord.second = 9000;
      g_ennemyBaseCoord.first = 0;
      g_ennemyBaseCoord.second = 0;
      g_ennemyBasePeripheryCoord.first = 1650;
      g_ennemyBasePeripheryCoord.second = 1650;
   }
   else
   {
      g_myBaseCoord.first = 0;
      g_myBaseCoord.second = 0;
      g_ennemyBaseCoord.first = 16000;
      g_ennemyBaseCoord.second = 9000;
      g_ennemyBasePeripheryCoord.first = 14350;
      g_ennemyBasePeripheryCoord.second = 7350;


   }

   for (int i = 0; i < 4; ++i)
   {
      for (int j = 0; j < 2; ++j)
      {
         if ((i == 0 && j == 0) || (i == 3 && j == 1)) continue; //our base and ennemy base we do not explore
         g_explorableTiles.push_back(ExplorableTile(16000 / 8 + 16000 / 4 * i, 9000 / 4 + 9000 / 2 * j));
      }
   }
   //(sqrt(2)-1)/(sqrt(2))*R
   g_lastResortExplorableTiles.push_back(ExplorableTile(2200, 6800));
   g_lastResortExplorableTiles.push_back(ExplorableTile(2200, 6800));
   g_lastResortExplorableTiles.push_back(ExplorableTile(1555, 7445));

   g_lastResortExplorableTiles.push_back(ExplorableTile(9400, 2200));
   g_lastResortExplorableTiles.push_back(ExplorableTile(13800, 2200));
   g_lastResortExplorableTiles.push_back(ExplorableTile(14445, 1555));

   for (int i = 0; i < g_bustersPerPlayer; ++i)
   {
      g_stunReloadTimes.push_back(0);
      g_assignedEnnemies[i] = -1;

   }
}

static void readOneTurn()
{
   cin >> g_entitiesCount; cin.ignore();
   for (int i = 0; i < g_entitiesCount; i++) {
      int entityId; // buster id or ghost id
      int x;
      int y; // position of this buster / ghost
      int entityType; // the team id if it is a buster, -1 if it is a ghost.
      int state; // For busters: 0=idle, 1=carrying a ghost.
      int value; // For busters: Ghost id being carried. For ghosts: number of busters attempting to trap this ghost.
      cin >> entityId >> x >> y >> entityType >> state >> value; cin.ignore();
      g_entities.push_back(Entity(entityId, x, y, entityType, state, value));
      g_visibleEntities[entityId] = Entity(entityId, x, y, entityType, state, value);
      print(g_entities.back(), false);
      if (entityType != -1 && state > 0)
      {
         //CAPTURED GHOST
         g_idsOfCapturedGhosts.push_back(state);
      }
      switch (entityType)
      {
         //ghost
      case -1:
         g_ghosts.push_back(Entity(entityId, x, y, entityType, state, value));
         break;
      case 0:
         if (g_myTeamId == 0)
         {
            g_myBusters.push_back(Entity(entityId, x, y, entityType, state, value));
         }
         else
         {
            Entity ennemy(entityId, x, y, entityType, state, value);
            g_hisBusters.push_back(ennemy);
            if (hasGhost(ennemy))
            {
               g_ennemiesWithGhosts.push_back(ennemy);
            }
         }
         for (auto explorableTile : g_explorableTiles)
         {
            if (computeDistance(g_entities.back(), explorableTile.m_x, explorableTile.m_y) <= 300)
            {
               explorableTile.m_explored = true;
               g_explorableTiles.erase(std::remove(g_explorableTiles.begin(), g_explorableTiles.end(), explorableTile), g_explorableTiles.end());
               //               g_explorableTiles.erase(explorableTile);
            }
         }
         if (g_lastResortExploration)
         {
            for (auto explorableTile : g_lastResortExplorableTiles)
            {

               if (computeDistance(g_entities.back(), explorableTile.m_x, explorableTile.m_y) <= 1)
               {
                  explorableTile.m_explored = true;
                  g_lastResortExplorableTiles.erase(std::remove(g_lastResortExplorableTiles.begin(), g_lastResortExplorableTiles.end(), explorableTile), g_lastResortExplorableTiles.end());
               }
            }
         }
         break;
      case 1:
         if (g_myTeamId == 1)
         {
            g_myBusters.push_back(Entity(entityId, x, y, entityType, state, value));
         }
         else
         {
            Entity ennemy(entityId, x, y, entityType, state, value);
            g_hisBusters.push_back(ennemy);
            if (hasGhost(ennemy))
            {
               g_ennemiesWithGhosts.push_back(ennemy);
            }
         }
         for (auto explorableTile : g_explorableTiles)
         {
            if (computeDistance(g_entities.back(), explorableTile.m_x, explorableTile.m_y) <= 300)
            {
               explorableTile.m_explored = true;
               g_explorableTiles.erase(std::remove(g_explorableTiles.begin(), g_explorableTiles.end(), explorableTile), g_explorableTiles.end());
               //               g_explorableTiles.erase(explorableTile);
            }
         }
         if (g_explorableTiles.empty())
         {
            for (auto explorableTile : g_lastResortExplorableTiles)
            {

               if (computeDistance(g_entities.back(), explorableTile.m_x, explorableTile.m_y) <= 1)
               {
                  explorableTile.m_explored = true;
                  g_lastResortExplorableTiles.erase(std::remove(g_lastResortExplorableTiles.begin(), g_lastResortExplorableTiles.end(), explorableTile), g_lastResortExplorableTiles.end());
               }
            }
         }
         break;
      default:
         throw;//err
         break;
      }
   }
   //remove captured ghosts
   remove_if(g_ghosts.begin(), g_ghosts.end(), isGhostCaptured);
   for (int i = 0; i < g_myBusters.size(); ++i)
   {
      Entity& buster = g_myBusters[i];
      buster.m_reloadTime = g_stunReloadTimes[i];
      buster.m_rank = i;
   }
   fillTrackingVector();
   printEnnemiesWithGhost();
   printAssignedEnnemies();
   //updateTrackingVector();//USELESS
}

static void doRelease()
{
   cout << "RELEASE" << endl;
}

static void doMove(int x, int y, const std::string& text = "")
{
   cout << "MOVE " << x << " " << y << " " << text << endl;
}

static void doBust(int ghostId)
{
   cout << "BUST " << ghostId << endl;
}

static void doStun(int busterId, const std::string& text)
{
   cout << "STUN " << busterId << " " << text << endl;
}

static pair<bool, Entity> selectClosestEnnemy(const Entity& buster)
{
   if (g_hisBusters.size() == 0) return make_pair(false, Entity());

   bool found = false;
   int minDistance = 32000;
   Entity closestEnnemy;
   for (auto ennemy : g_hisBusters)
   {
      int currentDistance = computeDistance(buster, ennemy);
      if (currentDistance < minDistance)
      {
         found = true;
         closestEnnemy = ennemy;
         minDistance = currentDistance;
      }
   }
   return make_pair(found, closestEnnemy);
}

static pair<bool, Entity> selectClosestStunnableEnnemy(const Entity& buster)
{
   if (g_hisBusters.size() == 0) return make_pair(false, Entity());

   bool found = false;
   int minDistance = 32000;
   Entity closestEnnemy;
   for (auto ennemy : g_hisBusters)
   {
      int currentDistance = computeDistance(buster, ennemy);
      if (canStun(buster, ennemy) && currentDistance < minDistance)
      {
         found = true;
         closestEnnemy = ennemy;
         minDistance = currentDistance;
      }
   }
   return make_pair(found, closestEnnemy);
}

static pair<bool, Entity> selectClosestEnnemyWithGhost(const Entity& buster)
{
   if (g_hisBusters.size() == 0) return make_pair(false, Entity());

   bool found = false;
   int minDistance = 32000;
   Entity closestEnnemy;
   for (auto ennemy : g_hisBusters)
   {
      int currentDistance = computeDistance(buster, ennemy);
      if (hasGhost(ennemy) && currentDistance < minDistance)
      {
         found = true;
         closestEnnemy = ennemy;
         minDistance = currentDistance;
      }
   }
   return make_pair(found, closestEnnemy);
}

static pair<int, int> selectClosestUnexploredTile(const Entity& entity, const std::vector<ExplorableTile>& tilesToExplore)
{
   int minDistance = 32000;
   pair<int, int> tilePos;
   for (auto currentTile : tilesToExplore)
   {
      int currentDistance = computeDistance(entity, currentTile.m_x, currentTile.m_y);
      if (currentDistance < minDistance)
      {
         tilePos = make_pair(currentTile.m_x, currentTile.m_y);
         minDistance = currentDistance;
      }
   }
   return tilePos;
}

static Entity selectClosestGhost(const Entity& buster)
{
   int minDistance = 32000;//should be > max
   Entity closestGhost;
   closestGhost.m_id = -1;
   for (auto ghost : g_ghosts)
   {
      int currentDistance = computeDistance(buster, ghost);
      if (currentDistance < minDistance)
      {
         minDistance = currentDistance;
         closestGhost = ghost;
      }
   }
   return closestGhost;
}

static Entity selectBestGhost(const Entity& buster)
{
   int maxScore = -500000;
   Entity bestGhost;
   bestGhost.m_id = -1;
   for (auto ghost : g_ghosts)
   {
      int currentDistance = computeDistance(buster, ghost);
      int currentEndurance = ghost.m_state;
      int currentScore = -currentDistance / 800 - pow(currentEndurance, 2);
      if (maxScore < currentScore)
      {
         maxScore = currentScore;
         bestGhost = ghost;
      }
   }
   return bestGhost;
}

static bool handleTrackEnnemyWithGhostSituation(Entity& myBuster)
{
   int ennemyId = g_assignedEnnemies[myBuster.m_rank];//potentially not visible anymore
   if (ennemyId != -1)
   {
      if (isStun(myBuster)) return false;

      const Entity& ennemy = g_visibleEntities[ennemyId];
      if (ennemy.m_id != -1 && canStun(myBuster, ennemy))
      {
         doStun(ennemy.m_id, "STUN T");
         g_assignedEnnemies[myBuster.m_rank] = -1;//TRACK COMPLETE
         g_assignedEnnemiesTargetPos[myBuster.m_rank] = make_pair(0, 0);//USELESS
      }
      else
      {
         pair<int, int> targetPos = g_assignedEnnemiesTargetPos[myBuster.m_rank];
         doMove(targetPos.first, targetPos.second, "TRACK");
      }
      return true;
   }
   return false;
}

static bool handleCarryGhostSituation(Entity& myBuster)
{
   if (hasGhost(myBuster))
   {
      if (canRelease(myBuster))
      {
         doRelease();
      }
      else
      {
         doMove(g_myBaseCoord.first, g_myBaseCoord.second, "CARRY");
      }
      return true;
   }
   return false;
}

static bool handleEnnemyCloseWithGhostSituation(Entity& myBuster)
{
   pair<bool, Entity> closestEnnemy = selectClosestEnnemyWithGhost(myBuster);
   if (closestEnnemy.first)//success
   {
      cerr << "FOUND CLOSEST WITH GHOST" << endl;
      if (canStun(myBuster, closestEnnemy.second))
      {
         g_stunReloadTimes[myBuster.m_rank] = 20;
         doStun(closestEnnemy.second.m_id, "STUN G");
         return true;
      }
   }
   return false;
}

static bool handleEnnemyCloseSituation(Entity& myBuster)
{
   pair<bool, Entity> closestEnnemy = selectClosestEnnemy(myBuster);
   if (closestEnnemy.first)//success
   {
      if (canStun(myBuster, closestEnnemy.second))
      {
         g_stunReloadTimes[myBuster.m_rank] = 20;
         doStun(closestEnnemy.second.m_id, "STUN C");
         return true;
      }
   }
   return false;
}

static void searchGhosts(const Entity& myBuster)
{
   pair<int, int> tilePos = selectClosestUnexploredTile(myBuster, g_explorableTiles);
   if (tilePos.first != 0 && tilePos.second != 0)
   {
      doMove(tilePos.first, tilePos.second, "SEARCH");
      return;
   }

   tilePos = selectClosestUnexploredTile(myBuster, g_lastResortExplorableTiles);
   if (tilePos.first != 0 && tilePos.second != 0)
   {
      g_lastResortExploration = true;
      doMove(tilePos.first, tilePos.second, "SEARCH2");
      return;
   }

   tilePos = make_pair(abs(2000 - g_ennemyBaseCoord.first), abs(2000 - g_ennemyBaseCoord.second));
   doMove(tilePos.first, tilePos.second, "NO MORE SEARCH");
   return;
}

static bool handleScoutingSituation(Entity& scout)
{
   searchGhosts(scout);
   return true;
}

static bool handleCantSeeGhostSituation(Entity& myBuster)
{
   if (!canSeeGhost())
   {
      searchGhosts(myBuster);
      return true;
   }
   return false;
}

static bool handleGhostCloseSituation(Entity& myBuster)
{
   const Entity& bestGhost = selectBestGhost(myBuster);
   if (canBust(myBuster, bestGhost))
   {
      doBust(bestGhost.m_id);
   }
   else if (computeDistance(myBuster, bestGhost) <= 900)
   {
      doMove(bestGhost.m_x - 400, bestGhost.m_y - 400);
   }
   else
   {
      doMove(bestGhost.m_x, bestGhost.m_y, "FOUND");
   }
   return true;
}

static void busterPlayOneTurn(Entity& myBuster)
{
   print(myBuster, false);
   bool done = false;
   done = handleCarryGhostSituation(myBuster);
   done = done || handleTrackEnnemyWithGhostSituation(myBuster);
   done = done || handleEnnemyCloseSituation(myBuster);
   done = done || handleCantSeeGhostSituation(myBuster);
   done = done || handleGhostCloseSituation(myBuster);
}

static void scoutPlayOneTurn(Entity& scout)
{
   bool done = false;
   done = handleCarryGhostSituation(scout);
   done = done || handleTrackEnnemyWithGhostSituation(scout);
   done = done || handleScoutingSituation(scout);
   //done = done || handleGhostCloseSituation(scout);
}

static void playOneTurn()
{
   for (int i = 0; i < g_bustersPerPlayer; i++) {

      // Write an action using cout. DON'T FORGET THE "<< endl"
      // To debug: cerr << "Debug messages..." << endl;
      if (false)//i == 0 && g_explorableTiles.size() != 0)
      {
         scoutPlayOneTurn(g_myBusters[i]);
      }
      else
      {
         busterPlayOneTurn(g_myBusters[i]);
      }
   }
}

static void onTurnEnd()
{
   g_entities.clear(); // a vector to store the entities
   g_ghosts.clear();
   g_myBusters.clear();
   g_hisBusters.clear();
   g_idsOfCapturedGhosts.clear();
   g_ennemiesWithGhosts.clear();
   g_visibleEntities.clear();
   for (int& reloadTime : g_stunReloadTimes)
   {
      reloadTime--;
      //cerr << "RELOAD TIME : " << reloadTime << endl;
   }
}

/**
* Send your busters out into the fog to trap ghosts and bring them home!
**/
int main()
{
   readGameSettings();

   // game loop
   while (1)
   {
      readOneTurn();
      playOneTurn();
      onTurnEnd();
   }
}