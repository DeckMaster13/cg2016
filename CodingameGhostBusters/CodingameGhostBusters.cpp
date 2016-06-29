#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <utility>
#include <array>

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
   int m_id; // buster id or ghost id
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

//CURRENT TURN VALUES
static int                    g_entitiesCount; // the number of busters and ghosts visible to you
static vector<Entity>         g_entities; // a vector to store the entities
static vector<Entity>         g_ghosts;
static vector<Entity>         g_myBusters;
static vector<Entity>         g_hisBusters;
static vector<int>            g_idsOfCapturedGhosts;
static vector<ExplorableTile> g_explorableTiles(4 * 2);
static vector<int>            g_stunReloadTimes;

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

static void readGameSettings()
{
   cin >> g_bustersPerPlayer; cin.ignore();
   cin >> g_ghostCount; cin.ignore();
   cin >> g_myTeamId; cin.ignore();
   if (g_myTeamId == 1)
   {
      g_myBaseCoord.first = 16000;
      g_myBaseCoord.second = 9000;
   }
   else
   {
      g_myBaseCoord.first = 0;
      g_myBaseCoord.second = 0;
   }

   for (int i = 0; i < 4; ++i)
   {
      for (int j = 0; j < 2; ++j)
      {
         if ((i == 0 && j == 0) || (i == 3 && j == 1)) continue; //our base and ennemy base we do not explore
         g_explorableTiles.push_back(ExplorableTile(16000 / 8 + 16000/4*i, 9000 / 4 + 9000/2*j));
      }
   }

   for (int i = 0; i < g_bustersPerPlayer; ++i)
   {
      g_stunReloadTimes.push_back(0);
   }

}

static bool isGhostCaptured(const Entity& ghost)
{
   for (auto id : g_idsOfCapturedGhosts)
   {
      if (id == ghost.m_id) return true;
   }
   return false;
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
            g_hisBusters.push_back(Entity(entityId, x, y, entityType, state, value));
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
         break;
      case 1:
         if (g_myTeamId == 1)
         {
            g_myBusters.push_back(Entity(entityId, x, y, entityType, state, value));
         }
         else
         {
            g_hisBusters.push_back(Entity(entityId, x, y, entityType, state, value));
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
}

static bool canSeeGhost()
{
   return g_ghosts.size() != 0;
}

static bool canStun(const Entity& buster, const Entity& ennemy)
{
   return buster.m_reloadTime <= 0 && computeDistance(buster, ennemy) <= 1760;
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

static bool hasGhost(const Entity& buster)
{
   return buster.m_state == 1;
}

static void doRelease()
{
   cout << "RELEASE" << endl;
}

static void doMove(int x, int y)
{
   cout << "MOVE " << x << " " << y << endl;
}

static void doBust(int ghostId)
{
   cout << "BUST " << ghostId << endl;
}

static void doStun(int busterId)
{
   cout << "STUN " << busterId << endl;
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

static pair<int, int> selectClosestUnexploredTile(const Entity& entity)
{
   int minDistance = 32000;
   pair<int, int> tilePos;
   for (auto currentTile : g_explorableTiles)
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

static bool handleCarryGhostSituation(Entity& myBuster)
{
   if (hasGhost(myBuster))
   {
      cerr << "HAS GHOST";
      if (canRelease(myBuster))
      {
         cerr << "--> RELEASE" << endl;
         doRelease();
      }
      else
      {
         cerr << "--> MOVE" << endl;
         doMove(g_myBaseCoord.first, g_myBaseCoord.second);
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
         cerr << "STUN" << endl;
         g_stunReloadTimes[myBuster.m_rank] = 20;
         doStun(closestEnnemy.second.m_id);
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
      cerr << "FOUND CLOSEST" << endl;
      if (canStun(myBuster, closestEnnemy.second))
      {
         cerr << "STUN" << endl;
         g_stunReloadTimes[myBuster.m_rank] = 20;
         doStun(closestEnnemy.second.m_id);
         return true;
      }
   }
   return false;
}

static bool handleCantSeeGhostSituation(Entity& myBuster)
{
   if (!canSeeGhost())
   {
      cerr << "CANT SEE GHOST" << endl;
      /*
      if (closestEnnemy.first)//success
      {
      int distanceBetweenClosestEnnemy = computeDistance(myBuster, closestEnnemy.second);
      if (distanceBetweenClosestEnnemy < 1500)
      {
      doMove(closestEnnemy.second.m_x, closestEnnemy.second.m_y);
      return;
      }
      }
      */

      pair<int, int> tilePos = selectClosestUnexploredTile(myBuster);
      cerr << "Closest tile: " << "x: " << tilePos.first << " y: " << tilePos.second << endl;
      //g_explorableTiles.erase(std::remove(g_explorableTiles.begin(), g_explorableTiles.end(), ExplorableTile(tilePos.first, tilePos.second)), g_explorableTiles.end());//ambitieux ?
      
      if (tilePos.first == 0 && tilePos.second == 0)
      {
         tilePos.first = abs(2000 - abs(g_myBaseCoord.first - 16000));
         tilePos.second = abs(2000 - abs(g_myBaseCoord.second - 9000));
      }

      doMove(tilePos.first, tilePos.second);
      //doMove(abs(g_myBaseCoord.first - 16000), abs(g_myBaseCoord.second - 9000));
      return true;
   }
   return false;
}

static bool handleGhostCloseSituation(Entity& myBuster)
{
   const Entity& closestGhost = selectClosestGhost(myBuster);
   if (canBust(myBuster, closestGhost))
   {
      cerr << "BUST" << endl;
      doBust(closestGhost.m_id);
   }
   else
   {
      cerr << "CANT BUST" << endl;
      doMove(closestGhost.m_x, closestGhost.m_y);
   }
   return true;
}

static void busterPlayOneTurn(Entity& myBuster)
{
   print(myBuster, false);
   bool done = false;
   done = handleCarryGhostSituation(myBuster);
   done = done || handleEnnemyCloseSituation(myBuster);
   done = done || handleCantSeeGhostSituation(myBuster);
   done = done || handleGhostCloseSituation(myBuster);
}

static void scoutPlayOneTurn(Entity& scout)
{
   bool done = false;
   done = handleCarryGhostSituation(scout);
   done = done || handleEnnemyCloseWithGhostSituation(scout);
   done = done || handleCantSeeGhostSituation(scout);
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
   for (int& reloadTime : g_stunReloadTimes)
   {
      reloadTime--;
      cerr << "RELOAD TIME : " << reloadTime << endl;
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