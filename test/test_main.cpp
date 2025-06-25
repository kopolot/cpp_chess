#include <gtest/gtest.h>

// Funkcja do przetestowania
int dodaj(int a, int b) {
    return a + b;
}

// Testy jednostkowe
TEST(DodawanieTest, DodajDwieLiczby) {
    EXPECT_EQ(dodaj(2, 3), 5);
    EXPECT_EQ(dodaj(-1, 1), 0);
    EXPECT_EQ(dodaj(0, 0), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}