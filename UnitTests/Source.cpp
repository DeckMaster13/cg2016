#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <utility>

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
   {}
   ~Entity() = default;
   Entity(const Entity&) = default;
public:
   int m_id; // buster id or ghost id
   int m_x;
   int m_y; // position of this buster / ghost
   int m_type; // the team id if it is a buster, -1 if it is a ghost.
   int m_state; // For busters: 0=idle, 1=carrying a ghost.
   int m_value; // For busters: Ghost id being carried. For ghosts: number of busters attempting to trap this ghost.
};

//OVERALL SETTINGS
static int g_bustersPerPlayer; // the amount of busters you control
static int g_ghostCount; // the amount of ghosts on the map
static int g_myTeamId; // if this is 0, your base is on the top left of the map, if it is one, on the bottom right
static pair<int, int> g_myBaseCoord; // coord of the base

//CURRENT TURN VALUES
static int g_entitiesCount; // the number of busters and ghosts visible to you
static vector<Entity> g_entities; // a vector to store the entities
static vector<Entity> g_ghosts;
static vector<Entity> g_myBusters;
static vector<Entity> g_hisBusters;

static void print(const Entity& e)
{
   cerr << " -- ENTITY -- " << endl;
   cerr << "id: " << e.m_id << endl;
   cerr << "x: " << e.m_x << endl;
   cerr << "y: " << e.m_y << endl;
   cerr << "type: " << e.m_type << endl;
   cerr << "state: " << e.m_state << endl;
   cerr << "value: " << e.m_value << endl;
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
      print(g_entities.back());
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
         break;
      default:
         throw;//err
         break;
      }
   }
}

static bool canSeeGhost()
{
   return g_ghosts.size() != 0;
}

static int computeDistanceFromBase(const Entity& e1)
{
   return static_cast<int>(sqrt(pow(e1.m_x - g_myBaseCoord.first, 2) + pow(e1.m_y - g_myBaseCoord.second, 2)));
}

static int computeDistance(const Entity& e1, const Entity& e2)
{
   return static_cast<int>(sqrt(pow(e1.m_x - e2.m_x, 2) + pow(e1.m_y - e2.m_y, 2)));
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

static void busterPlayOneTurn(const Entity& myBuster)
{
   if (hasGhost(myBuster))
   {
      if (canRelease(myBuster))
      {
         doRelease();
      }
      else
      {
         doMove(g_myBaseCoord.first, g_myBaseCoord.second);
      }
      return;
   }

   if (!canSeeGhost)
   {
      doMove(abs(g_myBaseCoord.first - 16000), abs(g_myBaseCoord.second - 9000));
      return;
   }

   const Entity& closestGhost = selectClosestGhost(myBuster);
   if (canBust(myBuster, closestGhost))
   {
      doBust(closestGhost.m_id);
   }
   else
   {
      doMove(closestGhost.m_x, closestGhost.m_y);
   }
}

static void playOneTurn()
{
   for (int i = 0; i < g_bustersPerPlayer; i++) {

      // Write an action using cout. DON'T FORGET THE "<< endl"
      // To debug: cerr << "Debug messages..." << endl;
      busterPlayOneTurn(g_myBusters[i]);
   }
}