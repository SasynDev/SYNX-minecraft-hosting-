-- ==============================================================================
-- SYNX HOSTING - Seed Data
-- Default Admin Account, Local Host Node Definition, Plans, and Initial Port Pool
-- ==============================================================================

-- 1. Default Administrator Account
-- Password: "Admin@Synx2026!" (Bcrypt hash: $2a$12$ZqjZ3tXjM7.64D61Jb.lZOCmQ1cQYd1aI1L08f0/UvR9YkMh21d2C)
INSERT INTO users (id, username, email, password_hash, role, mfa_enabled, is_active)
VALUES (
    'a0000000-0000-0000-0000-000000000001',
    'admin',
    'admin@synx.local',
    '$2a$12$ZqjZ3tXjM7.64D61Jb.lZOCmQ1cQYd1aI1L08f0/UvR9YkMh21d2C',
    'admin',
    FALSE,
    TRUE
) ON CONFLICT (username) DO NOTHING;

-- 2. Primary Local Host Node
INSERT INTO nodes (
    id,
    name,
    description,
    fqdn,
    ip_address,
    daemon_port,
    daemon_token_hash,
    memory_total_mb,
    memory_allocated_mb,
    disk_total_mb,
    disk_allocated_mb,
    cpu_cores,
    status,
    is_master
) VALUES (
    'b0000000-0000-0000-0000-000000000001',
    'localhost-minecraft-server',
    'Localhost Minecraft Server (Windows 11 Bare-Metal)',
    'localhost',
    '127.0.0.1',
    8443,
    'synx_agent_dev_token_hash_default_secret_998244353',
    8192,
    0,
    512000,
    0,
    12,
    'online',
    TRUE
) ON CONFLICT (name) DO NOTHING;

-- 3. Default Hosting Plans
INSERT INTO server_plans (id, name, description, ram_limit_mb, cpu_limit_pct, disk_limit_mb, max_backups, max_allocations)
VALUES
    ('c0000000-0000-0000-0000-000000000001', 'Starter SMP', 'Perfect for small groups of friends (up to 5 players)', 1024, 75, 10240, 2, 1),
    ('c0000000-0000-0000-0000-000000000002', 'Standard SMP', 'Ideal for Paper/Purpur community servers with plugins', 2048, 120, 20480, 3, 2),
    ('c0000000-0000-0000-0000-000000000003', 'Pro Geyser Crossplay', 'High-performance Paper + GeyserMC & Floodgate', 4096, 200, 40960, 5, 3),
    ('c0000000-0000-0000-0000-000000000004', 'Elite Modded', 'Heavy Forge/Fabric modpacks with expanded JVM headroom', 6144, 300, 61440, 7, 4)
ON CONFLICT (name) DO NOTHING;

-- 4. Initial Network Allocations (Java Edition: 25565 - 25585 on 127.0.0.1)
INSERT INTO network_allocations (node_id, ip_address, port, protocol, is_primary, notes)
SELECT
    'b0000000-0000-0000-0000-000000000001',
    '127.0.0.1',
    p,
    'tcp',
    (p = 25565),
    CASE WHEN p = 25565 THEN 'Default Minecraft Port' ELSE 'Dynamic Allocation' END
FROM generate_series(25565, 25585) AS p
ON CONFLICT (node_id, ip_address, port) DO NOTHING;

-- 5. Initial Network Allocations (Bedrock Edition UDP: 19132 - 19142 on 127.0.0.1)
INSERT INTO network_allocations (node_id, ip_address, port, protocol, is_primary, notes)
SELECT
    'b0000000-0000-0000-0000-000000000001',
    '127.0.0.1',
    p,
    'udp',
    (p = 19132),
    CASE WHEN p = 19132 THEN 'Default Bedrock Port' ELSE 'Dynamic Bedrock Port' END
FROM generate_series(19132, 19142) AS p
ON CONFLICT (node_id, ip_address, port) DO NOTHING;

