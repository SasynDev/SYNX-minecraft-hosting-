-- ==============================================================================
-- SYNX HOSTING - Master PostgreSQL Database Schema
-- Architecture: Production-Ready Relational Model for Game Orchestration
-- ==============================================================================

-- Enable UUID extension
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
CREATE EXTENSION IF NOT EXISTS "pgcrypto";

-- ==============================================================================
-- 1. ENUMS & DOMAINS
-- ==============================================================================

DO $$ BEGIN
    CREATE TYPE user_role AS ENUM ('admin', 'support', 'customer');
EXCEPTION
    WHEN duplicate_object THEN null;
END $$;

DO $$ BEGIN
    CREATE TYPE node_status AS ENUM ('online', 'offline', 'maintenance', 'unreachable');
EXCEPTION
    WHEN duplicate_object THEN null;
END $$;

DO $$ BEGIN
    CREATE TYPE instance_status AS ENUM (
        'stopped',
        'starting',
        'running',
        'stopping',
        'crashed',
        'installing',
        'suspended',
        'error'
    );
EXCEPTION
    WHEN duplicate_object THEN null;
END $$;

DO $$ BEGIN
    CREATE TYPE server_type AS ENUM (
        'paper',
        'purpur',
        'vanilla',
        'fabric',
        'forge',
        'bedrock'
    );
EXCEPTION
    WHEN duplicate_object THEN null;
END $$;

DO $$ BEGIN
    CREATE TYPE backup_status AS ENUM ('in_progress', 'completed', 'failed', 'restoring');
EXCEPTION
    WHEN duplicate_object THEN null;
END $$;

DO $$ BEGIN
    CREATE TYPE storage_type AS ENUM ('local', 's3', 'remote_node');
EXCEPTION
    WHEN duplicate_object THEN null;
END $$;

DO $$ BEGIN
    CREATE TYPE protocol_type AS ENUM ('tcp', 'udp', 'both');
EXCEPTION
    WHEN duplicate_object THEN null;
END $$;

-- ==============================================================================
-- 2. USERS & AUTHENTICATION
-- ==============================================================================

CREATE TABLE IF NOT EXISTS users (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    username VARCHAR(50) NOT NULL UNIQUE,
    email VARCHAR(255) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    role user_role NOT NULL DEFAULT 'customer',
    mfa_enabled BOOLEAN NOT NULL DEFAULT FALSE,
    mfa_secret VARCHAR(64) NULL,
    is_active BOOLEAN NOT NULL DEFAULT TRUE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS user_sessions (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    refresh_token_hash VARCHAR(64) NOT NULL UNIQUE,
    user_agent TEXT NULL,
    ip_address INET NULL,
    expires_at TIMESTAMPTZ NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- ==============================================================================
-- 3. HOSTING INFRASTRUCTURE & NODES
-- ==============================================================================

CREATE TABLE IF NOT EXISTS nodes (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    name VARCHAR(100) NOT NULL UNIQUE,
    description TEXT NULL,
    fqdn VARCHAR(255) NOT NULL,
    ip_address INET NOT NULL,
    daemon_port INTEGER NOT NULL DEFAULT 8443,
    daemon_token_hash VARCHAR(255) NOT NULL,
    memory_total_mb INTEGER NOT NULL,
    memory_allocated_mb INTEGER NOT NULL DEFAULT 0,
    disk_total_mb BIGINT NOT NULL,
    disk_allocated_mb BIGINT NOT NULL DEFAULT 0,
    cpu_cores INTEGER NOT NULL,
    status node_status NOT NULL DEFAULT 'online',
    is_master BOOLEAN NOT NULL DEFAULT FALSE,
    heartbeat_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- ==============================================================================
-- 4. SERVER PLANS & TIERS
-- ==============================================================================

CREATE TABLE IF NOT EXISTS server_plans (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    name VARCHAR(100) NOT NULL UNIQUE,
    description TEXT NULL,
    ram_limit_mb INTEGER NOT NULL,
    cpu_limit_pct INTEGER NOT NULL, -- e.g. 100 = 1 full core, 200 = 2 cores
    disk_limit_mb BIGINT NOT NULL,
    max_backups INTEGER NOT NULL DEFAULT 3,
    max_allocations INTEGER NOT NULL DEFAULT 1,
    is_public BOOLEAN NOT NULL DEFAULT TRUE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- ==============================================================================
-- 5. NETWORK ALLOCATIONS (PORT POOLS)
-- ==============================================================================

CREATE TABLE IF NOT EXISTS network_allocations (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    node_id UUID NOT NULL REFERENCES nodes(id) ON DELETE CASCADE,
    ip_address INET NOT NULL,
    port INTEGER NOT NULL CHECK (port BETWEEN 1024 AND 65535),
    protocol protocol_type NOT NULL DEFAULT 'tcp',
    instance_id UUID NULL, -- Assigned instance, NULL if available
    is_primary BOOLEAN NOT NULL DEFAULT FALSE,
    notes VARCHAR(255) NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT uq_node_ip_port UNIQUE (node_id, ip_address, port)
);

-- ==============================================================================
-- 6. MINECRAFT INSTANCES (CORE ORCHESTRATION ENTITY)
-- ==============================================================================

CREATE TABLE IF NOT EXISTS minecraft_instances (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    identifier VARCHAR(8) NOT NULL UNIQUE, -- Short reference slug e.g. 'mc-8f3e1a'
    name VARCHAR(100) NOT NULL,
    description TEXT NULL,
    owner_id UUID NOT NULL REFERENCES users(id) ON DELETE RESTRICT,
    node_id UUID NOT NULL REFERENCES nodes(id) ON DELETE RESTRICT,
    plan_id UUID NULL REFERENCES server_plans(id) ON DELETE SET NULL,
    server_type server_type NOT NULL DEFAULT 'paper',
    mc_version VARCHAR(32) NOT NULL DEFAULT '1.21.4',
    java_version INTEGER NOT NULL DEFAULT 21,
    ram_limit_mb INTEGER NOT NULL CHECK (ram_limit_mb >= 512),
    cpu_limit_pct INTEGER NOT NULL DEFAULT 100 CHECK (cpu_limit_pct >= 25),
    disk_limit_mb BIGINT NOT NULL CHECK (disk_limit_mb >= 1024),
    status instance_status NOT NULL DEFAULT 'stopped',
    primary_allocation_id UUID NULL REFERENCES network_allocations(id) ON DELETE SET NULL,
    rcon_port INTEGER NULL,
    rcon_password_enc VARCHAR(255) NULL,
    crossplay_enabled BOOLEAN NOT NULL DEFAULT FALSE,
    bedrock_port INTEGER NULL,
    cracked_enabled BOOLEAN NOT NULL DEFAULT TRUE, -- Allows offline-mode / cracked players (e.g. TLauncher)
    auto_restart BOOLEAN NOT NULL DEFAULT TRUE,
    crash_count INTEGER NOT NULL DEFAULT 0,
    startup_command TEXT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- Add foreign key constraint to link allocation back to instance safely
ALTER TABLE network_allocations
    DROP CONSTRAINT IF EXISTS fk_allocation_instance,
    ADD CONSTRAINT fk_allocation_instance
    FOREIGN KEY (instance_id) REFERENCES minecraft_instances(id) ON DELETE SET NULL;

-- ==============================================================================
-- 7. INSTANCE ENVIRONMENT & STARTUP VARIABLES
-- ==============================================================================

CREATE TABLE IF NOT EXISTS instance_variables (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    instance_id UUID NOT NULL REFERENCES minecraft_instances(id) ON DELETE CASCADE,
    env_key VARCHAR(100) NOT NULL,
    env_value TEXT NOT NULL,
    is_user_editable BOOLEAN NOT NULL DEFAULT TRUE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT uq_instance_env_key UNIQUE (instance_id, env_key)
);

-- ==============================================================================
-- 8. BACKUPS SYSTEM
-- ==============================================================================

CREATE TABLE IF NOT EXISTS backups (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    instance_id UUID NOT NULL REFERENCES minecraft_instances(id) ON DELETE CASCADE,
    name VARCHAR(150) NOT NULL,
    filename VARCHAR(255) NOT NULL,
    size_bytes BIGINT NOT NULL DEFAULT 0,
    checksum_sha256 VARCHAR(64) NULL,
    storage storage_type NOT NULL DEFAULT 'local',
    status backup_status NOT NULL DEFAULT 'in_progress',
    error_message TEXT NULL,
    completed_at TIMESTAMPTZ NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- ==============================================================================
-- 9. AUDIT LOGGING & SECURITY EVENTS
-- ==============================================================================

CREATE TABLE IF NOT EXISTS audit_logs (
    id BIGSERIAL PRIMARY KEY,
    user_id UUID NULL REFERENCES users(id) ON DELETE SET NULL,
    action VARCHAR(100) NOT NULL,
    target_type VARCHAR(50) NOT NULL, -- e.g. 'instance', 'user', 'node', 'file'
    target_id VARCHAR(100) NULL,
    ip_address INET NULL,
    user_agent TEXT NULL,
    metadata JSONB NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS security_firewall_rules (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    node_id UUID NOT NULL REFERENCES nodes(id) ON DELETE CASCADE,
    port INTEGER NOT NULL,
    protocol protocol_type NOT NULL DEFAULT 'tcp',
    action VARCHAR(10) NOT NULL CHECK (action IN ('allow', 'deny')),
    source_cidr CIDR NOT NULL DEFAULT '0.0.0.0/0',
    description VARCHAR(255) NULL,
    is_active BOOLEAN NOT NULL DEFAULT TRUE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- ==============================================================================
-- 10. INDEXES FOR PERFORMANCE
-- ==============================================================================

CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
CREATE INDEX IF NOT EXISTS idx_instances_owner ON minecraft_instances(owner_id);
CREATE INDEX IF NOT EXISTS idx_instances_node ON minecraft_instances(node_id);
CREATE INDEX IF NOT EXISTS idx_instances_status ON minecraft_instances(status);
CREATE INDEX IF NOT EXISTS idx_instances_identifier ON minecraft_instances(identifier);
CREATE INDEX IF NOT EXISTS idx_allocations_available ON network_allocations(node_id, instance_id) WHERE instance_id IS NULL;
CREATE INDEX IF NOT EXISTS idx_backups_instance ON backups(instance_id);
CREATE INDEX IF NOT EXISTS idx_audit_logs_user ON audit_logs(user_id);
CREATE INDEX IF NOT EXISTS idx_audit_logs_target ON audit_logs(target_type, target_id);
CREATE INDEX IF NOT EXISTS idx_audit_logs_created ON audit_logs(created_at DESC);

-- ==============================================================================
-- 11. AUTOMATIC UPDATED_AT TIMESTAMP TRIGGER
-- ==============================================================================

CREATE OR REPLACE FUNCTION update_timestamp_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS trg_users_updated_at ON users;
CREATE TRIGGER trg_users_updated_at
BEFORE UPDATE ON users
FOR EACH ROW EXECUTE FUNCTION update_timestamp_column();

DROP TRIGGER IF EXISTS trg_nodes_updated_at ON nodes;
CREATE TRIGGER trg_nodes_updated_at
BEFORE UPDATE ON nodes
FOR EACH ROW EXECUTE FUNCTION update_timestamp_column();

DROP TRIGGER IF EXISTS trg_instances_updated_at ON minecraft_instances;
CREATE TRIGGER trg_instances_updated_at
BEFORE UPDATE ON minecraft_instances
FOR EACH ROW EXECUTE FUNCTION update_timestamp_column();

