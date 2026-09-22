-- LFU with invalid-line preference, FIFO tie-break

function chooseVictim(lines, request, cache)
    -- First prefer any invalid line
    for i, line in ipairs(lines) do
        if not line.valid then
            return i
        end
    end

    -- Otherwise choose least frequently used
    local victim = 1

    for i = 2, #lines do
        if lines[i].frequency < lines[victim].frequency then
            victim = i

        elseif lines[i].frequency == lines[victim].frequency then
            -- FIFO tie-break: older insert_time wins
            if lines[i].insertTime < lines[victim].insertTime then
                victim = i
            end
        end
    end

    return victim
end


function onAccess(way, hit)
    -- Optional hook
end


function onInsert(way)
    -- Optional hook
end


function onEvict(way)
    -- Optional hook
end