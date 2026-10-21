// 运行时驻留 API 单元测试（任务 1272·闭包扫描后截断回baseline）
// 不变量：截断保留 ID 0..keep；被截断条目从内容映射移除；同内容重新驻留获得
//   相同 ID（扫描→截断→完整编译的 Symbol 分配序列稳定）；baseline已有条目指针
//   在截断后仍有效（deque 不搬移）。
#include <gtest/gtest.h>

#include <string>

extern "C" long long __cn_intern(const char* text);
extern "C" const char* __cn_intern_text(long long id);
extern "C" long long __cn_intern_count();
extern "C" void __cn_intern_truncate(unsigned long long keep);

TEST(InternApiTest, TruncateAndReinternStable) {
    const long long baseline = __cn_intern_count();
    const char* textA = "1272截断测试·idA";
    const char* textB = "1272截断测试·idB";
    const long long idA = __cn_intern(textA);
    ASSERT_GT(idA, baseline);
    const char* ptrA = __cn_intern_text(idA);
    ASSERT_NE(ptrA, nullptr);
    const long long idB = __cn_intern(textB);
    ASSERT_GT(idB, idA);

    // 截断到idA：idB被移除，idA保留；baseline已有指针仍有效
    __cn_intern_truncate(static_cast<unsigned long long>(idA));
    EXPECT_EQ(__cn_intern_count(), idA);
    EXPECT_STREQ(__cn_intern_text(idA), textA);
    EXPECT_STREQ(ptrA, textA);  // 截断不使已保留条目的文本指针失效

    // 同内容重新驻留获得相同 ID（分配序列稳定）
    const long long idB2 = __cn_intern(textB);
    EXPECT_EQ(idB2, idB);
    EXPECT_STREQ(__cn_intern_text(idB2), textB);

    // 收尾回到baseline（不影响其他测试的驻留假设）
    __cn_intern_truncate(static_cast<unsigned long long>(baseline));
    EXPECT_EQ(__cn_intern_count(), baseline);
}

TEST(InternApiTest, TruncateBeyondCountIsNoOp) {
    const long long count = __cn_intern_count();
    __cn_intern_truncate(static_cast<unsigned long long>(count + 100));
    EXPECT_EQ(__cn_intern_count(), count);
}
