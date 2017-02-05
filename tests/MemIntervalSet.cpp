#include "photon/gc/MemIntervalSet.h"

#include <gtest/gtest.h>

using namespace photon;

namespace std {

void PrintTo(const std::vector<MemInterval>& vec, ::std::ostream* os)
{
    ::std::ostream& stream = *os;
    if (vec.empty()) {
        stream << "{}";
        return;
    }
    stream << "{";
    for (auto it = vec.begin(); it < (vec.end() - 1); it++) {
        stream << "{" << it->start() << ", " << it->end() << "}, ";
    }
    stream << "{" << vec.back().start() << ", " << vec.back().end() << "}}";
}
}

void expectMerge(MemInterval* from, MemInterval* to, std::size_t start, std::size_t end)
{
    EXPECT_EQ(IntervalComparison::Intersects, from->mergeIntoIfIntersects(to));
    EXPECT_EQ(start, to->start());
    EXPECT_EQ(end, to->end());
}

void expectBefore(MemInterval* from, MemInterval* to)
{
    MemInterval result = *to;
    EXPECT_EQ(IntervalComparison::Before, from->mergeIntoIfIntersects(to));
    EXPECT_EQ(result.start(), to->start());
    EXPECT_EQ(result.end(), to->end());
}

void expectAfter(MemInterval* from, MemInterval* to)
{
    MemInterval result = *to;
    EXPECT_EQ(IntervalComparison::After, from->mergeIntoIfIntersects(to));
    EXPECT_EQ(result.start(), to->start());
    EXPECT_EQ(result.end(), to->end());
}


TEST(MemIntervalTest, equal)
{
    MemInterval from(20, 30);
    MemInterval to(20, 30);

    expectMerge(&from, &to, 20, 30);
}

TEST(MemIntervalTest, firstBeforeSecond)
{
    MemInterval from(20, 30);
    MemInterval to(40, 50);

    expectBefore(&from, &to);
}

TEST(MemIntervalTest, firstTouchesSecondLeft)
{
    MemInterval from(20, 30);
    MemInterval to(30, 40);

    expectMerge(&from, &to, 20, 40);
}

TEST(MemIntervalTest, firstOverlapsSecond)
{
    MemInterval from(50, 80);
    MemInterval to(70, 90);

    expectMerge(&from, &to, 50, 90);
}

TEST(MemIntervalTest, firstTouchesSecondRight)
{
    MemInterval from(20, 100);
    MemInterval to(70, 100);

    expectMerge(&from, &to, 20, 100);
}

TEST(MemIntervalTest, firstContainsSecond)
{
    MemInterval from(40, 90);
    MemInterval to(60, 80);

    expectMerge(&from, &to, 40, 90);
}

TEST(MemIntervalTest, secondTouchedFirstLeft)
{
    MemInterval from(10, 60);
    MemInterval to(10, 40);

    expectMerge(&from, &to, 10, 60);
}

TEST(MemIntervalTest, secondContainsFirst)
{
    MemInterval from(10, 30);
    MemInterval to(0, 40);

    expectMerge(&from, &to, 0, 40);
}

TEST(MemIntervalTest, secondOverlapsFirst)
{
    MemInterval from(30, 90);
    MemInterval to(20, 80);

    expectMerge(&from, &to, 20, 90);
}

TEST(MemIntervalTest, secondTouchedFirstRight)
{
    MemInterval from(60, 100);
    MemInterval to(20, 80);

    expectMerge(&from, &to, 20, 100);
}

TEST(MemIntervalTest, secondBeforeFirst)
{
    MemInterval from(50, 110);
    MemInterval to(20, 30);

    expectAfter(&from, &to);
}

class MemIntervalSetTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        _vec.clear();
    }

    void init(std::initializer_list<MemInterval> lst)
    {
        std::vector<MemInterval> vec(lst);
        _vec = MemIntervalSet(lst);
    }

    void init(const std::vector<MemInterval>& lst)
    {
        _vec = MemIntervalSet(lst);
    }

    void expect(std::initializer_list<MemInterval> lst)
    {
        EXPECT_EQ(std::vector<MemInterval>(lst), _vec.intervals());
    }

    void add(MemInterval os)
    {
        _vec.add(os);
    }

    MemIntervalSet _vec;
};

TEST_F(MemIntervalSetTest, empty)
{
    add({10, 20});
    expect({{10, 20}});
}

TEST_F(MemIntervalSetTest, beforeAll)
{
    init({{20, 30}, {40, 50}});
    add({0, 10});
    expect({{0, 10}, {20, 30}, {40, 50}});
}

TEST_F(MemIntervalSetTest, beforeFirstIntFirst)
{
    init({{20, 30}, {40, 50}});
    add({0, 21});
    expect({{0, 30}, {40, 50}});
}

TEST_F(MemIntervalSetTest, beforeFirstIntSecond)
{
    init({{20, 30}, {40, 50}});
    add({0, 41});
    expect({{0, 50}});
}

TEST_F(MemIntervalSetTest, beforeSecond)
{
    init({{10, 50}, {100, 120}});
    add({60, 70});
    expect({{10, 50}, {60, 70}, {100, 120}});
}

TEST_F(MemIntervalSetTest, beforeSecondIntSecond)
{
    init({{10, 50}, {100, 120}});
    add({60, 101});
    expect({{10, 50}, {60, 120}});
}

TEST_F(MemIntervalSetTest, afterLast)
{
    init({{10, 50}, {100, 120}});
    add({150, 160});
    expect({{10, 50}, {100, 120}, {150, 160}});
}

TEST_F(MemIntervalSetTest, intersectSeveral)
{
    init({{10, 20}, {30, 40}, {50, 60}, {70, 80}});
    add({35, 55});
    expect({{10, 20}, {30, 60}, {70, 80}});
}

TEST_F(MemIntervalSetTest, intersectAll)
{
    init({{10, 20}, {30, 40}, {50, 60}, {70, 80}});
    add({0, 75});
    expect({{0, 80}});
}

TEST_F(MemIntervalSetTest, intersectSeveralBig)
{
    std::vector<MemInterval> vec;
    for (std::size_t size = 0; size < 1000; size += 20) {
        vec.emplace_back(size, size + 10);
    }
    init(vec);
    add({25, 975});
    expect({{0, 10}, {20, 975}, {980, 990}});
}
