//
// Created by mohammad on 5/22/23.
//

#include "urlparser.h"
#include <gtest/gtest.h>

TEST(MyTestSuite, MyTestCase) {
  EXPECT_EQ("a.b.c.zarebin.ir",
            TLD::Url("https://a.b.c.zarebin.ir/").fulldomain());
  //    EXPECT_EQ("zarebin.ir", TLD::Url("zarebin.ir").fulldomain());
  //    EXPECT_EQ("www.zarebin.ir", TLD::Url("www.zarebin.ir").fulldomain());
  //    EXPECT_EQ("subdomain.zarebin.ir",
  //    TLD::Url("subdomain.zarebin.ir").fulldomain());
  EXPECT_EQ("subdomain.zarebin.ir",
            TLD::Url("https://subdomain.zarebin.ir").fulldomain());
  EXPECT_EQ("subdomain.zarebin.ir",
            TLD::Url("https://subdomain.zarebin.ir/").fulldomain());
  EXPECT_EQ("subdomain.zarebin.ir",
            TLD::Url("https://subdomain.zarebin.ir/path").fulldomain());
  EXPECT_EQ("subdomain.zarebin.ir",
            TLD::Url("https://subdomain.zarebin.ir/path?q=1").fulldomain());
  //    EXPECT_EQ("subdomain.zarebin.ir",
  //    TLD::Url("subdomain.zarebin.ir/path?q=1").fulldomain());
  //    EXPECT_EQ("zarebin.ir", TLD::Url("zarebin.ir/path?q=1").fulldomain());
  //    EXPECT_EQ("zarebin.ir", TLD::Url("zarebin.ir:2020").fulldomain());
  //    EXPECT_EQ("www.zarebin.ir",
  //    TLD::Url("www.zarebin.ir:8888").fulldomain());
  EXPECT_EQ(
      "subdomain.zarebin.ir",
      TLD::Url("https://subdomain.zarebin.ir:8080/path?q=1").fulldomain());
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
