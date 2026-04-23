#include <unity.h>
#include "audio/AudioCommandQueue.h"

using namespace pixelroot32::audio;

void setUp(void) {}
void tearDown(void) {}

void test_audio_command_queue_initial_state(void) {
    AudioCommandQueue queue;
    TEST_ASSERT_TRUE(queue.isEmpty());
}

void test_audio_command_queue_enqueue_dequeue(void) {
    AudioCommandQueue queue;
    AudioCommand cmd1;
    cmd1.type = AudioCommandType::PLAY_EVENT;
    cmd1.event.type = WaveType::PULSE;
    cmd1.event.frequency = 440.0f;
    cmd1.event.duration = 0.5f;
    cmd1.event.volume = 1.0f;
    
    TEST_ASSERT_TRUE(queue.enqueue(cmd1));
    TEST_ASSERT_FALSE(queue.isEmpty());
    
    AudioCommand outCmd;
    TEST_ASSERT_TRUE(queue.dequeue(outCmd));
    TEST_ASSERT_TRUE(queue.isEmpty());
    TEST_ASSERT_EQUAL(AudioCommandType::PLAY_EVENT, outCmd.type);
    TEST_ASSERT_EQUAL_FLOAT(440.0f, outCmd.event.frequency);
}

void test_audio_command_queue_full(void) {
    AudioCommandQueue queue;
    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 0.5f;
    cmd.event.volume = 0.8f;
    
    // Fill the queue. CAPACITY is 128, but ring buffer with one slot reserved for full/empty distinction?
    // Looking at the code: nextTail == head means full. So max CAPACITY-1 items.
    for (size_t i = 0; i < AudioCommandQueue::CAPACITY - 1; i++) {
        TEST_ASSERT_TRUE(queue.enqueue(cmd));
    }
    
    // Next enqueue should fail
    TEST_ASSERT_FALSE(queue.enqueue(cmd));
}

void test_audio_command_queue_empty_dequeue(void) {
    AudioCommandQueue queue;
    AudioCommand outCmd;
    TEST_ASSERT_FALSE(queue.dequeue(outCmd));
}

// MPSC tests - Multiple Producer Single Consumer
void test_audio_command_queue_mpsc_single_producer_baseline(void) {
    AudioCommandQueue queue;
    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 0.5f;
    cmd.event.volume = 1.0f;
    
    // Single producer should work same as before
    for (size_t i = 0; i < 64; i++) {
        TEST_ASSERT_TRUE(queue.enqueue(cmd));
    }
    
    // Dequeue all 64
    AudioCommand outCmd;
    size_t dequeued = 0;
    while (queue.dequeue(outCmd)) {
        dequeued++;
    }
    TEST_ASSERT_EQUAL(64, dequeued);
}

void test_audio_command_queue_mpsc_full_with_drops(void) {
    AudioCommandQueue queue;
    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 0.5f;
    cmd.event.volume = 0.8f;
    
    // Fill to capacity
    for (size_t i = 0; i < AudioCommandQueue::CAPACITY - 1; i++) {
        queue.enqueue(cmd);
    }
    
    // Get dropped count - should be 0 initially
    TEST_ASSERT_EQUAL(0, queue.getDroppedCommands());
    
    // Try to enqueue more - should fail and increment dropped
    bool enqueueResult = queue.enqueue(cmd);
    TEST_ASSERT_FALSE(enqueueResult); // Queue full, drop policy
    
    // Now dropped count should reflect the drop
    TEST_ASSERT_TRUE(queue.getDroppedCommands() > 0);
}

void test_audio_command_queue_mpsc_drop_counter_accuracy(void) {
    AudioCommandQueue queue;
    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 0.5f;
    cmd.event.volume = 0.8f;

    // Reset dropped counter by creating new queue
    AudioCommandQueue queue2;
    TEST_ASSERT_EQUAL(0, queue2.getDroppedCommands());

    // Fill the queue to capacity (CAPACITY-1 items max)
    for (size_t i = 0; i < AudioCommandQueue::CAPACITY - 1; i++) {
        queue2.enqueue(cmd);
    }

    // Try to overfill - each failed enqueue should increment dropped
    // Now try to add more than capacity
    for (size_t i = 0; i < 10; i++) {
        queue2.enqueue(cmd); // These will fail and increment dropped
    }

    // Dropped should be at least 10 (we tried to add 10 to full queue)
    TEST_ASSERT_TRUE(queue2.getDroppedCommands() >= 10);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_audio_command_queue_initial_state);
    RUN_TEST(test_audio_command_queue_enqueue_dequeue);
    RUN_TEST(test_audio_command_queue_full);
    RUN_TEST(test_audio_command_queue_empty_dequeue);
    // MPSC tests
    RUN_TEST(test_audio_command_queue_mpsc_single_producer_baseline);
    RUN_TEST(test_audio_command_queue_mpsc_full_with_drops);
    RUN_TEST(test_audio_command_queue_mpsc_drop_counter_accuracy);
    return UNITY_END();
}
