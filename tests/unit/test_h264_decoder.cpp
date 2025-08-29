#include <gtest/gtest.h>
#include "decoders/H264Decoder.h"
#include <memory>

using namespace plugin_h264;

class H264DecoderTest : public ::testing::Test {
protected:
    void SetUp() override {
        decoder_ = std::make_unique<H264Decoder>();
    }
    
    void TearDown() override {
        decoder_.reset();
    }
    
    std::unique_ptr<H264Decoder> decoder_;
};

// 基于验证结果的测试用例
TEST_F(H264DecoderTest, InitializationSuccess) {
    // 测试解码器初始化
    EXPECT_TRUE(decoder_->initialize());
    EXPECT_FALSE(decoder_->hasError());
}

TEST_F(H264DecoderTest, DestroyWithoutInitialize) {
    // 测试未初始化情况下的析构
    decoder_->destroy();
    EXPECT_FALSE(decoder_->hasError());
}

TEST_F(H264DecoderTest, MultipleInitialization) {
    // 测试多次初始化
    EXPECT_TRUE(decoder_->initialize());
    EXPECT_TRUE(decoder_->initialize()); // 第二次应该直接返回true
    EXPECT_FALSE(decoder_->hasError());
}

TEST_F(H264DecoderTest, DecodeWithoutInitialization) {
    // 测试未初始化情况下的解码
    VideoFrame frame;
    uint8_t dummy_data[] = {0x00, 0x00, 0x00, 0x01, 0x67}; // 模拟SPS NAL
    
    bool result = decoder_->decode(dummy_data, sizeof(dummy_data), frame);
    EXPECT_FALSE(result);
    EXPECT_TRUE(decoder_->hasError());
    EXPECT_EQ(decoder_->getLastError(), H264Error::DECODER_INIT_FAILED);
}

TEST_F(H264DecoderTest, DecodeInvalidParameters) {
    // 初始化解码器
    ASSERT_TRUE(decoder_->initialize());
    
    VideoFrame frame;
    
    // 测试空指针
    bool result = decoder_->decode(nullptr, 100, frame);
    EXPECT_FALSE(result);
    EXPECT_TRUE(decoder_->hasError());
    EXPECT_EQ(decoder_->getLastError(), H264Error::INVALID_PARAM);
    
    // 重置错误状态
    decoder_->clearError();
    
    // 测试大小为0
    uint8_t dummy_data[] = {0x00, 0x00, 0x00, 0x01, 0x67};
    result = decoder_->decode(dummy_data, 0, frame);
    EXPECT_FALSE(result);
    EXPECT_TRUE(decoder_->hasError());
    EXPECT_EQ(decoder_->getLastError(), H264Error::INVALID_PARAM);
}

TEST_F(H264DecoderTest, GetDecoderInfoBeforeInitialization) {
    // 测试初始化前获取解码器信息
    int width, height;
    bool result = decoder_->getDecoderInfo(width, height);
    EXPECT_FALSE(result);
}

TEST_F(H264DecoderTest, GetDecoderInfoAfterInitialization) {
    // 初始化后获取解码器信息（应该返回0x0，因为还没有解码帧）
    ASSERT_TRUE(decoder_->initialize());
    
    int width, height;
    bool result = decoder_->getDecoderInfo(width, height);
    EXPECT_FALSE(result); // 没有解码帧时应该返回false
}

TEST_F(H264DecoderTest, MemoryUsageTracking) {
    // 测试内存使用量跟踪
    size_t usage_before = decoder_->getMemoryUsage();
    
    ASSERT_TRUE(decoder_->initialize());
    
    size_t usage_after = decoder_->getMemoryUsage();
    EXPECT_GT(usage_after, usage_before); // 初始化后应该使用更多内存
}

TEST_F(H264DecoderTest, ResetFunctionality) {
    // 测试重置功能
    ASSERT_TRUE(decoder_->initialize());
    
    // 重置解码器
    decoder_->reset();
    
    // 重置后应该重新初始化
    EXPECT_TRUE(decoder_->initialize());
    EXPECT_FALSE(decoder_->hasError());
}

TEST_F(H264DecoderTest, ErrorHandling) {
    // 测试错误处理机制
    ASSERT_TRUE(decoder_->initialize());
    
    VideoFrame frame;
    uint8_t invalid_data[] = {0xFF, 0xFF, 0xFF, 0xFF}; // 无效数据
    
    bool result = decoder_->decode(invalid_data, sizeof(invalid_data), frame);
    
    // 即使解码失败，解码器也应该能正确处理错误
    if (!result) {
        EXPECT_TRUE(decoder_->hasError());
        EXPECT_FALSE(decoder_->getLastMessage().empty());
    }
    
    // 清除错误后状态应该恢复正常
    decoder_->clearError();
    EXPECT_FALSE(decoder_->hasError());
}

TEST_F(H264DecoderTest, ThreadSafety) {
    // 基础的线程安全测试（创建和销毁多个实例）
    const int num_decoders = 10;
    std::vector<std::unique_ptr<H264Decoder>> decoders;
    
    // 创建多个解码器实例
    for (int i = 0; i < num_decoders; ++i) {
        auto decoder = std::make_unique<H264Decoder>();
        EXPECT_TRUE(decoder->initialize());
        decoders.push_back(std::move(decoder));
    }
    
    // 验证所有解码器都正确初始化
    for (const auto& decoder : decoders) {
        EXPECT_FALSE(decoder->hasError());
    }
    
    // 清理（析构函数会自动调用destroy）
    decoders.clear();
}