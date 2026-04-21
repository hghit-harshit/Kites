# Custom Cache Replacement Policy Documentation

## Overview

The RISC-V Simulator supports custom cache replacement policies through Lua scripts. This allows you to implement your own cache eviction strategy without modifying the core simulator code.

The custom policy system provides a complete view of the cache state and lets you hook into key cache lifecycle events:
- **Victim Selection**: Decide which cache line to evict when a miss occurs
- **Access Handling**: React to cache hits and misses
- **Insertion Tracking**: Handle new lines brought into the cache
- **Eviction Tracking**: Handle lines being removed from the cache

---

## Cache Data Exposed

### CacheLineView - Individual Cache Line Information

Each cache line in the system provides the following read-only data:

```lua
line = {
    valid       : boolean,    -- Is this cache line currently valid?
    tag         : uint64,     -- The tag bits of the memory address in this line
    age         : uint64,     -- Age counter (cycles since insertion)
    frequency   : uint64,     -- Access frequency counter (hit count)
    insertTime  : uint64,     -- Timestamp when this line was inserted into the cache
    lastAccess  : uint64,     -- Timestamp of the most recent access
    dirty       : boolean     -- Has this line been modified (write-back policy)?
}
```

**Field Explanations:**
- **valid**: `true` if the cache line contains valid data, `false` if it's empty
- **tag**: The upper bits of the memory address associated with this line
- **age**: Incremented each cycle; useful for age-based replacement (e.g., LRU approximation)
- **frequency**: Incremented on each access hit; useful for LFU-based replacement
- **insertTime**: Global timestamp when the line was brought into the cache
- **lastAccess**: Global timestamp of the most recent hit on this line
- **dirty**: `true` if the line has been modified but not written back to memory (relevant for write-back caches)

---

### CacheRequestView - Current Memory Access Information

Information about the memory access being processed:

```lua
request = {
    address     : uint64,     -- Full memory address of the access
    setIndex    : size_t,     -- Index of the cache set (for set-associative caches)
    wayIndex    : size_t,     -- Way/line index within the set
    offset      : size_t,     -- Byte offset within the cache block
    accessSize  : size_t,     -- Size of the access (1, 2, 4, 8 bytes)
    isWrite     : boolean,    -- Is this a write operation?
    tag         : uint64      -- Tag bits extracted from the address
}
```

**Field Explanations:**
- **address**: The full 64-bit memory address being accessed
- **setIndex**: Which cache set is being accessed (0 to `cache.setCount-1`)
- **wayIndex**: Which way/line within the set (0 to `cache.wayCount-1`)
- **offset**: Byte position within the 64-byte cache block (0-63)
- **accessSize**: How many bytes are being read/written (typically 1, 2, 4, or 8)
- **isWrite**: `true` for write operations, `false` for reads
- **tag**: The tag extracted from the address that gets stored in the cache line

---

### CacheContextView - Overall Cache Configuration

Information about the cache structure and timing:

```lua
cache = {
    setCount    : size_t,     -- Number of cache sets
    wayCount    : size_t,     -- Number of ways per set (associativity)
    blockSize   : size_t,     -- Size of a cache block in bytes (typically 64)
    tick        : uint64      -- Current global timestamp/cycle counter
}
```

**Field Explanations:**
- **setCount**: Total number of sets in the cache (cache_size / (blockSize * wayCount))
- **wayCount**: Associativity of the cache (1 for direct-mapped, 2 for 2-way, etc.)
- **blockSize**: Size of each cache block in bytes (typically 64 bytes)
- **tick**: Current cycle counter; increments each memory access for age tracking

**Example Configuration:**
For a 8KB 4-way set-associative cache with 64-byte blocks:
- `setCount = 32` (8192 / 64 / 4)
- `wayCount = 4`
- `blockSize = 64`

---

## Required Lua Functions

Your Lua script must implement the following functions. The `chooseVictim` function is **required**, while the other three are **optional** hooks.

### 1. chooseVictim (REQUIRED)

**Purpose**: Select a victim cache line to evict when a miss occurs.

**Signature**:
```lua
function chooseVictim(lines, request, cache)
    -- Your implementation here
    return victimIndex  -- Return value: integer (0 to cache.wayCount-1)
end
```

**Parameters**:
- **lines** (`table`): Array of `CacheLineView` tables for all lines in the current set
  - Access as: `lines[1]`, `lines[2]`, ..., `lines[cache.wayCount]`
  - Each line has the fields: `valid`, `tag`, `age`, `frequency`, `insertTime`, `lastAccess`, `dirty`
  
- **request** (`table`): Information about the current cache access
  - Fields: `address`, `setIndex`, `wayIndex`, `offset`, `accessSize`, `isWrite`, `tag`
  
- **cache** (`table`): Cache configuration and state
  - Fields: `setCount`, `wayCount`, `blockSize`, `tick`

**Return Value**:
- **Integer**: Index of the victim line to evict (1-indexed in Lua, 0 to `cache.wayCount-1`)
- Must be a valid index within the `lines` array
- Typically, prefer invalid lines (where `line.valid == false`)

**Example: Simple LRU Replacement**
```lua
function chooseVictim(lines, request, cache)
    local victim = 1
    local oldest = lines[1].lastAccess
    
    for i = 2, cache.wayCount do
        if lines[i].lastAccess < oldest then
            oldest = lines[i].lastAccess
            victim = i
        end
    end
    
    return victim
end
```

**Example: Prefer Invalid Lines**
```lua
function chooseVictim(lines, request, cache)
    -- First, prefer any empty/invalid line
    for i = 1, cache.wayCount do
        if not lines[i].valid then
            return i
        end
    end
    
    -- If no invalid line, use LRU
    local victim = 1
    for i = 2, cache.wayCount do
        if lines[i].lastAccess < lines[victim].lastAccess then
            victim = i
        end
    end
    
    return victim
end
```

---

### 2. onAccess (OPTIONAL)

**Purpose**: Hook called after a cache hit/miss is processed. Used to track access patterns.

**Signature**:
```lua
function onAccess(lines, request, cache)
    -- Optional implementation for tracking access patterns
end
```

**Parameters**:
- **lines** (`table`): Array of `CacheLineView` for the current set
- **request** (`table`): The memory access that triggered this hook
- **cache** (`table`): Cache configuration

**Usage Examples**:
- Update frequency counters for LFU policies
- Track access patterns for predictive prefetching
- Log cache access statistics

**Example: Frequency-Based Tracking**
```lua
function onAccess(lines, request, cache)
    -- This would be called after each access
    -- You can use the frequency field to implement LFU
end
```

---

### 3. onInsert (OPTIONAL)

**Purpose**: Hook called when a new line is brought into the cache. Used to initialize line metadata.

**Signature**:
```lua
function onInsert(lines, request, cache)
    -- Optional implementation for new line insertion
end
```

**Parameters**:
- **lines** (`table`): Array of `CacheLineView` for the current set (includes the newly inserted line)
- **request** (`table`): The memory access that caused this insertion
- **cache** (`table`): Cache configuration

**Usage Examples**:
- Initialize frequency counters to 1 for new lines
- Track allocation patterns
- Implement prefetch-aware insertion policies

**Example: Initialize Frequency**
```lua
function onInsert(lines, request, cache)
    -- Could be used to track when lines are first brought in
    -- The line already has insertTime set by the cache controller
end
```

---

### 4. onEvict (OPTIONAL)

**Purpose**: Hook called when a line is evicted/removed from the cache. Used for cleanup and statistics.

**Signature**:
```lua
function onEvict(lines, request, cache)
    -- Optional implementation for line eviction
end
```

**Parameters**:
- **lines** (`table`): Array of `CacheLineView` for the current set (before eviction)
- **request** (`table`): The memory access that triggered this eviction
- **cache** (`table`): Cache configuration

**Usage Examples**:
- Update eviction statistics
- Track dirty line evictions
- Implement adaptive policies based on eviction patterns

**Example: Track Dirty Evictions**
```lua
function onEvict(lines, request, cache)
    -- Access lines[request.wayIndex] to see what's being evicted
    local victimLine = lines[request.wayIndex]
    if victimLine.dirty then
        -- Count dirty evictions for statistics
    end
end
```

---

## Complete Example: LFU with FIFO Tie-Breaking

Here's a complete working example implementing Least Frequently Used (LFU) replacement with FIFO tie-breaking for lines with equal frequency:

```lua
-- File: lfu.lua
-- LFU with invalid-line preference, FIFO tie-break

function chooseVictim(lines, request, cache)
    -- First prefer any invalid line
    for i = 1, cache.wayCount do
        if not lines[i].valid then
            return i
        end
    end

    -- Otherwise choose least frequently used
    local victim = 1
    local minFrequency = lines[1].frequency

    for i = 2, cache.wayCount do
        if lines[i].frequency < minFrequency then
            minFrequency = lines[i].frequency
            victim = i
        elseif lines[i].frequency == minFrequency then
            -- FIFO tie-break: older insert_time wins
            if lines[i].insertTime < lines[victim].insertTime then
                victim = i
            end
        end
    end

    return victim
end

-- Optional hook: track access frequency
function onAccess(lines, request, cache)
    -- The frequency field is already updated by the cache controller
    -- You could use this to implement custom access tracking
end

-- Optional hook: initialize new insertions
function onInsert(lines, request, cache)
    -- Called when a line is brought into the cache
    -- The insertTime and frequency are already initialized
end

-- Optional hook: track evictions
function onEvict(lines, request, cache)
    -- Called when a line is evicted
    -- You could log statistics here
end
```

---

## Complete Example: Age-Based Pseudo-LRU

Here's an implementation using the `age` field for efficient LRU approximation:

```lua
-- File: pseudo_lru.lua
-- Simple age-based replacement

function chooseVictim(lines, request, cache)
    -- Prefer invalid lines
    for i = 1, cache.wayCount do
        if not lines[i].valid then
            return i
        end
    end

    -- Choose line with highest age (oldest)
    local victim = 1
    local maxAge = lines[1].age

    for i = 2, cache.wayCount do
        if lines[i].age > maxAge then
            maxAge = lines[i].age
            victim = i
        end
    end

    return victim
end
```

---
