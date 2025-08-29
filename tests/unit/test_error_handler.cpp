#include <gtest/gtest.h>
#include "utils/ErrorHandler.h"

using namespace plugin_h264;

class ErrorHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        handler_ = std::make_unique<ErrorHandler>();
    }
    
    std::unique_ptr<ErrorHandler> handler_;
};

TEST_F(ErrorHandlerTest, InitialState) {
    // 初始状态应该没有错误
    EXPECT_FALSE(handler_->hasError());
    EXPECT_EQ(handler_->getLastError(), H264Error::NONE);
    EXPECT_TRUE(handler_->getLastMessage().empty());
}

TEST_F(ErrorHandlerTest, SetAndGetError) {
    // 设置错误
    const std::string test_message = "Test error message";
    handler_->setError(H264Error::DECODE_FAILED, test_message);
    
    EXPECT_TRUE(handler_->hasError());
    EXPECT_EQ(handler_->getLastError(), H264Error::DECODE_FAILED);
    EXPECT_EQ(handler_->getLastMessage(), test_message);
}

TEST_F(ErrorHandlerTest, ClearError) {
    // 设置错误后清除
    handler_->setError(H264Error::INVALID_PARAM, "Test");
    EXPECT_TRUE(handler_->hasError());
    
    handler_->clearError();
    EXPECT_FALSE(handler_->hasError());
    EXPECT_EQ(handler_->getLastError(), H264Error::NONE);
    EXPECT_TRUE(handler_->getLastMessage().empty());
}

TEST_F(ErrorHandlerTest, ErrorToString) {
    // 测试错误码到字符串的转换
    EXPECT_STREQ(ErrorHandler::errorToString(H264Error::NONE), "No error");
    EXPECT_STREQ(ErrorHandler::errorToString(H264Error::INVALID_PARAM), "Invalid parameter");
    EXPECT_STREQ(ErrorHandler::errorToString(H264Error::DECODE_FAILED), "Decode operation failed");
    EXPECT_STREQ(ErrorHandler::errorToString(H264Error::FILE_NOT_FOUND), "File not found");
}

TEST_F(ErrorHandlerTest, OverwriteError) {
    // 测试错误覆盖
    handler_->setError(H264Error::INVALID_PARAM, "First error");
    handler_->setError(H264Error::DECODE_FAILED, "Second error");
    
    EXPECT_EQ(handler_->getLastError(), H264Error::DECODE_FAILED);
    EXPECT_EQ(handler_->getLastMessage(), "Second error");
}

TEST_F(ErrorHandlerTest, EmptyMessage) {
    // 测试空消息
    handler_->setError(H264Error::OUT_OF_MEMORY, "");
    
    EXPECT_TRUE(handler_->hasError());
    EXPECT_EQ(handler_->getLastError(), H264Error::OUT_OF_MEMORY);
    EXPECT_TRUE(handler_->getLastMessage().empty());
}