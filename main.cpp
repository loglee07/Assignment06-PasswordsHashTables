#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

// Required educational hash function.
// This is not secure enough for real passwords.
uint64_t demoHash(const std::string& s) {
    uint64_t h = 1469598103934665603ULL;

    for (unsigned char c : s) {
        h = (h ^ c) * 1099511628211ULL;
    }

    return h;
}

// The pepper is used for every password but is not stored in each user.
const std::string PEPPER = "CS210-Class-Pepper";

struct UserRecord {
    std::string username;
    std::string salt;
    uint64_t passwordHash;
};

// Creates a user and stores the hash instead of the actual password.
UserRecord makeUser(
    const std::string& username,
    const std::string& password,
    const std::string& salt) {

    UserRecord user;
    user.username = username;
    user.salt = salt;
    user.passwordHash = demoHash(salt + password + PEPPER);

    return user;
}

enum class CollisionStrategy {
    Chaining,
    LinearProbing,
    QuadraticProbing,
    DoubleHashing
};

std::string strategyName(CollisionStrategy strategy) {
    if (strategy == CollisionStrategy::Chaining) {
        return "Separate Chaining";
    }

    if (strategy == CollisionStrategy::LinearProbing) {
        return "Linear Probing";
    }

    if (strategy == CollisionStrategy::QuadraticProbing) {
        return "Quadratic Probing";
    }

    return "Double Hashing";
}

// Prime-number capacities help the probing methods.
bool isPrime(std::size_t number) {
    if (number < 2) {
        return false;
    }

    for (std::size_t i = 2; i * i <= number; i++) {
        if (number % i == 0) {
            return false;
        }
    }

    return true;
}

std::size_t nextPrime(std::size_t number) {
    while (!isPrime(number)) {
        number++;
    }

    return number;
}

enum class SlotState {
    Empty,
    Occupied,
    Deleted
};

struct TableEntry {
    UserRecord user;
    SlotState state = SlotState::Empty;
};

class HashTable {
private:
    CollisionStrategy strategy;
    std::size_t capacity;
    std::size_t userCount;

    // Used by separate chaining.
    std::vector<std::vector<UserRecord>> buckets;

    // Used by the three probing methods.
    std::vector<TableEntry> entries;

    std::size_t firstHash(const std::string& username) const {
        return demoHash(username) % capacity;
    }

    std::size_t secondHash(const std::string& username) const {
        return 1 +
               (demoHash(username + "#second") % (capacity - 1));
    }

    std::size_t probeIndex(
        const std::string& username,
        std::size_t attempt) const {

        std::size_t start = firstHash(username);

        if (strategy == CollisionStrategy::LinearProbing) {
            return (start + attempt) % capacity;
        }

        if (strategy == CollisionStrategy::QuadraticProbing) {
            return (start + attempt * attempt) % capacity;
        }

        std::size_t step = secondHash(username);
        return (start + attempt * step) % capacity;
    }

    void resetStorage() {
        buckets.clear();
        entries.clear();

        if (strategy == CollisionStrategy::Chaining) {
            buckets.resize(capacity);
        } else {
            entries.resize(capacity);
        }
    }

    UserRecord* findMutable(const std::string& username) {
        if (strategy == CollisionStrategy::Chaining) {
            std::size_t index = firstHash(username);

            for (UserRecord& user : buckets[index]) {
                if (user.username == username) {
                    return &user;
                }
            }

            return nullptr;
        }

        for (std::size_t attempt = 0;
             attempt < capacity;
             attempt++) {

            std::size_t index =
                probeIndex(username, attempt);

            if (entries[index].state == SlotState::Empty) {
                return nullptr;
            }

            if (entries[index].state == SlotState::Occupied &&
                entries[index].user.username == username) {

                return &entries[index].user;
            }
        }

        return nullptr;
    }

    bool placeUser(
        const UserRecord& user,
        bool showCollision) {

        // Separate chaining places colliding users
        // inside the same bucket.
        if (strategy == CollisionStrategy::Chaining) {
            std::size_t index =
                firstHash(user.username);

            if (showCollision &&
                !buckets[index].empty()) {

                std::cout
                    << "Collision: "
                    << user.username
                    << " shares bucket "
                    << index
                    << ".\n";
            }

            buckets[index].push_back(user);
            userCount++;

            return true;
        }

        std::size_t firstDeleted = capacity;
        bool collisionPrinted = false;

        for (std::size_t attempt = 0;
             attempt < capacity;
             attempt++) {

            std::size_t index =
                probeIndex(user.username, attempt);

            if (entries[index].state ==
                SlotState::Occupied) {

                if (showCollision &&
                    !collisionPrinted) {

                    std::cout
                        << "Collision: "
                        << user.username
                        << " found occupied index "
                        << index
                        << " and continued probing.\n";

                    collisionPrinted = true;
                }

                continue;
            }

            if (entries[index].state ==
                SlotState::Deleted) {

                if (firstDeleted == capacity) {
                    firstDeleted = index;
                }

                continue;
            }

            if (firstDeleted != capacity) {
                index = firstDeleted;
            }

            entries[index].user = user;
            entries[index].state =
                SlotState::Occupied;

            userCount++;
            return true;
        }

        if (firstDeleted != capacity) {
            entries[firstDeleted].user = user;
            entries[firstDeleted].state =
                SlotState::Occupied;

            userCount++;
            return true;
        }

        return false;
    }

    std::vector<UserRecord> getAllUsers() const {
        std::vector<UserRecord> users;

        if (strategy == CollisionStrategy::Chaining) {
            for (const auto& bucket : buckets) {
                for (const UserRecord& user : bucket) {
                    users.push_back(user);
                }
            }
        } else {
            for (const TableEntry& entry : entries) {
                if (entry.state ==
                    SlotState::Occupied) {

                    users.push_back(entry.user);
                }
            }
        }

        return users;
    }

    void rehash() {
        std::vector<UserRecord> oldUsers =
            getAllUsers();

        std::size_t oldCapacity = capacity;

        capacity = nextPrime(capacity * 2);
        userCount = 0;

        resetStorage();

        for (const UserRecord& user : oldUsers) {
            placeUser(user, false);
        }

        std::cout
            << "Rehash event: capacity changed from "
            << oldCapacity
            << " to "
            << capacity
            << ".\n";
    }

public:
    HashTable(
        CollisionStrategy selectedStrategy,
        std::size_t startingCapacity = 7) {

        strategy = selectedStrategy;
        capacity = nextPrime(startingCapacity);
        userCount = 0;

        resetStorage();
    }

    void insert(const UserRecord& user) {
        // Update an existing username.
        UserRecord* existingUser =
            findMutable(user.username);

        if (existingUser != nullptr) {
            *existingUser = user;
            return;
        }

        // Resize before becoming more than 70% full.
        double futureLoad =
            static_cast<double>(userCount + 1) /
            capacity;

        if (futureLoad > 0.70) {
            rehash();
        }

        // If probing cannot locate a space,
        // resize and try again.
        if (!placeUser(user, true)) {
            rehash();
            placeUser(user, true);
        }
    }

    const UserRecord* find(
        const std::string& username) const {

        if (strategy ==
            CollisionStrategy::Chaining) {

            std::size_t index =
                firstHash(username);

            for (const UserRecord& user :
                 buckets[index]) {

                if (user.username == username) {
                    return &user;
                }
            }

            return nullptr;
        }

        for (std::size_t attempt = 0;
             attempt < capacity;
             attempt++) {

            std::size_t index =
                probeIndex(username, attempt);

            if (entries[index].state ==
                SlotState::Empty) {

                return nullptr;
            }

            if (entries[index].state ==
                    SlotState::Occupied &&
                entries[index].user.username ==
                    username) {

                return &entries[index].user;
            }
        }

        return nullptr;
    }

    bool remove(const std::string& username) {
        if (strategy ==
            CollisionStrategy::Chaining) {

            std::size_t index =
                firstHash(username);

            for (std::size_t i = 0;
                 i < buckets[index].size();
                 i++) {

                if (buckets[index][i].username ==
                    username) {

                    buckets[index].erase(
                        buckets[index].begin() + i);

                    userCount--;
                    return true;
                }
            }

            return false;
        }

        for (std::size_t attempt = 0;
             attempt < capacity;
             attempt++) {

            std::size_t index =
                probeIndex(username, attempt);

            if (entries[index].state ==
                SlotState::Empty) {

                return false;
            }

            if (entries[index].state ==
                    SlotState::Occupied &&
                entries[index].user.username ==
                    username) {

                entries[index].state =
                    SlotState::Deleted;

                userCount--;
                return true;
            }
        }

        return false;
    }

    double loadFactor() const {
        return static_cast<double>(userCount) /
               capacity;
    }

    std::size_t size() const {
        return userCount;
    }

    void printUsers() const {
        std::cout << "Stored users: ";

        std::vector<UserRecord> users =
            getAllUsers();

        for (const UserRecord& user : users) {
            std::cout << user.username << " ";
        }

        std::cout << '\n';
    }
};

bool login(
    const HashTable& table,
    const std::string& username,
    const std::string& submittedPassword) {

    const UserRecord* user =
        table.find(username);

    if (user == nullptr) {
        std::cout
            << "Login for "
            << username
            << ": unknown user\n";

        return false;
    }

    uint64_t submittedHash =
        demoHash(
            user->salt +
            submittedPassword +
            PEPPER);

    if (submittedHash ==
        user->passwordHash) {

        std::cout
            << "Login for "
            << username
            << ": successful\n";

        return true;
    }

    std::cout
        << "Login for "
        << username
        << ": incorrect password\n";

    return false;
}

void addFiveUsers(HashTable& table) {
    table.insert(
        makeUser(
            "alice",
            "apple123",
            "salt_alice_81"));

    table.insert(
        makeUser(
            "bob",
            "blue456",
            "salt_bob_27"));

    table.insert(
        makeUser(
            "carol",
            "cat789",
            "salt_carol_63"));

    table.insert(
        makeUser(
            "dave",
            "dog321",
            "salt_dave_45"));

    table.insert(
        makeUser(
            "erin",
            "eagle654",
            "salt_erin_92"));
}

void testCollisionStrategy(
    CollisionStrategy strategy) {

    std::cout
        << "\n===== "
        << strategyName(strategy)
        << " =====\n";

    HashTable table(strategy, 7);

    addFiveUsers(table);

    std::cout
        << "Users stored: "
        << table.size()
        << '\n';

    std::cout
        << "Load factor: "
        << std::fixed
        << std::setprecision(2)
        << table.loadFactor()
        << '\n';

    std::cout
        << "Find carol: "
        << (table.find("carol") != nullptr
                ? "Found"
                : "Not found")
        << '\n';

    std::cout
        << "Remove bob: "
        << (table.remove("bob")
                ? "Removed"
                : "Not found")
        << '\n';

    std::cout
        << "Find bob after removal: "
        << (table.find("bob") != nullptr
                ? "Found"
                : "Not found")
        << '\n';
}

int main() {
    std::cout << "COLLISION STRATEGY TESTS\n";

    testCollisionStrategy(
        CollisionStrategy::Chaining);

    testCollisionStrategy(
        CollisionStrategy::LinearProbing);

    testCollisionStrategy(
        CollisionStrategy::QuadraticProbing);

    testCollisionStrategy(
        CollisionStrategy::DoubleHashing);

    std::cout
        << "\n===== CHOSEN METHOD: "
        << "SEPARATE CHAINING =====\n";

    std::cout
        << "Separate chaining was selected because "
        << "collisions are easy to handle and "
        << "removal is straightforward.\n";

    HashTable authenticationTable(
        CollisionStrategy::Chaining,
        7);

    addFiveUsers(authenticationTable);
    authenticationTable.printUsers();

    std::cout << "\n===== LOGIN TESTS =====\n";

    // Successful login
    login(
        authenticationTable,
        "alice",
        "apple123");

    // Failed password
    login(
        authenticationTable,
        "alice",
        "wrongPassword");

    // Unknown user
    login(
        authenticationTable,
        "unknown",
        "anything");

    std::cout << "\n===== REMOVAL TEST =====\n";

    std::cout
        << "Remove dave: "
        << (authenticationTable.remove("dave")
                ? "Removed successfully"
                : "User not found")
        << '\n';

    // Dave should now be unknown.
    login(
        authenticationTable,
        "dave",
        "dog321");

    std::cout
        << "\nFinal load factor: "
        << authenticationTable.loadFactor()
        << '\n';

    std::cout
        << "\nSecurity note: demoHash is only "
        << "for classroom use.\n";

    std::cout
        << "Real systems should use a slow "
        << "password-hashing algorithm such as "
        << "Argon2, bcrypt, or scrypt.\n";

    return 0;
}