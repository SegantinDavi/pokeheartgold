/**
 * @file test_ring_buffer.cpp
 * @brief Unit tests for thread-safe circular audio ring buffer.
 */

#include "test_runner.h"
#include "audio/audio_backend.h"
#include <vector>

TEST_CASE(Audio_RingBuffer_BasicWriteRead) {
    PortAudio::AudioRingBuffer rb(1024);

    ASSERT_EQ(0, rb.AvailableReadFrames());
    ASSERT_EQ(1024, rb.AvailableWriteFrames());

    // Prepare 100 stereo sample frames
    std::vector<int16_t> input(200);
    for (size_t i = 0; i < input.size(); ++i) {
        input[i] = static_cast<int16_t>(i + 1);
    }

    size_t written = rb.Write(input.data(), 100);
    ASSERT_EQ(100, written);
    ASSERT_EQ(100, rb.AvailableReadFrames());
    ASSERT_EQ(924, rb.AvailableWriteFrames());

    // Read back 100 frames
    std::vector<int16_t> output(200, 0);
    size_t readCount = rb.Read(output.data(), 100);
    ASSERT_EQ(100, readCount);
    ASSERT_EQ(0, rb.AvailableReadFrames());

    for (size_t i = 0; i < input.size(); ++i) {
        ASSERT_EQ(input[i], output[i]);
    }
}

TEST_CASE(Audio_RingBuffer_Wraparound) {
    PortAudio::AudioRingBuffer rb(64);

    std::vector<int16_t> chunk(100); // 50 frames
    for (size_t i = 0; i < chunk.size(); ++i) {
        chunk[i] = static_cast<int16_t>(i);
    }

    // Write 50 frames, read 40 -> write ptr at 50, read ptr at 40
    rb.Write(chunk.data(), 50);
    std::vector<int16_t> out(80);
    rb.Read(out.data(), 40);

    // Write another 40 frames -> wraps around index 64
    size_t written = rb.Write(chunk.data(), 40);
    ASSERT_EQ(40, written);
    ASSERT_EQ(50, rb.AvailableReadFrames());

    // Read remaining 50 frames
    std::vector<int16_t> outAll(100);
    size_t totalRead = rb.Read(outAll.data(), 50);
    ASSERT_EQ(50, totalRead);
    ASSERT_EQ(0, rb.AvailableReadFrames());
}

TEST_CASE(Audio_RingBuffer_UnderrunSilence) {
    PortAudio::AudioRingBuffer rb(128);

    std::vector<int16_t> samples(20, 1000); // 10 frames of 1000
    rb.Write(samples.data(), 10);

    // Request 20 frames (only 10 available)
    std::vector<int16_t> out(40, -1);
    size_t readCount = rb.Read(out.data(), 20);
    ASSERT_EQ(10, readCount);

    // First 10 frames should match samples
    for (size_t i = 0; i < 20; ++i) {
        ASSERT_EQ(1000, out[i]);
    }
    // Next 10 frames should be filled with silence (0)
    for (size_t i = 20; i < 40; ++i) {
        ASSERT_EQ(0, out[i]);
    }
}
