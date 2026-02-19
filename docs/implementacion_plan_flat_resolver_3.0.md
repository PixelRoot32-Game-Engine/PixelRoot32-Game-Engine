Implementation Plan: Flat Solver v3.0
Upgrade the physics system to version 3.0, focusing on stability (Contact Persistence, Micro-Impulse) and performance (Sleep System).

Proposed Changes
[Component] Physics Core Abstractions
[MODIFY] 
CollisionTypes.h
Add Contact struct to store persistence data (IDs, Normal, Penetration, Accumulated Impulse).
Increase kMaxPairs in EngineConfig.h if necessary, but keep it fixed-size.
[Component] Actor System
[MODIFY] 
PhysicsActor.h
Add bool isSleeping and uint8_t sleepTimer.
Add LinearFriction and AngularFriction (for future) constants.
[MODIFY] 
PhysicsActor.cpp
Update 
update()
 to skip integration if isSleeping is true.
Implement wake/sleep transition logic based on velocity threshold.
[Component] Collision System "Flat Solver"
[MODIFY] 
CollisionSystem.h
Add static Contact contactCache[kMaxContacts].
Add internal methods for contact matching.
[MODIFY] 
CollisionSystem.cpp
Phase 1: Update 
update()
 to match current pairs against contactCache.
Phase 2: Implement applyImpulse pass after relaxation.
Phase 3: Implement the "Sleep" skip logic in the grid insertion and broadphase.
Phase 4: Add sweptAABB check for actors exceeding the velocity threshold.
Verification Plan
Automated Tests
PhysicsDemoScene: Validate that a stack of 10 boxes/circles remains stable without drifting or vibrating (v3.0 goal).
Manual Verification
Observe "Sleep" behavior in the debug overlay (active vs total objects).
Stress test with high-speed projectiles to verify the Swept AABB anti-tunneling.

Comment
Ctrl+Alt+M
