export type ServerType = 'paper' | 'purpur' | 'vanilla' | 'fabric' | 'forge' | 'bedrock';
export type InstanceStatus = 'stopped' | 'starting' | 'running' | 'stopping' | 'crashed' | 'installing' | 'error';

export interface InstanceSummary {
  id: string;
  identifier: string;
  name: string;
  server_type: ServerType;
  mc_version: string;
  port: number;
  cracked_enabled: boolean;
  ram_limit_mb: number;
  cpu_limit_pct: number;
  status: InstanceStatus;
}

export interface InstanceDetail extends InstanceSummary {
  ram_used_mb: number;
  cpu_usage_pct: number;
  disk_used_mb: number;
  uptime_seconds: number;
}

export interface HostMetrics {
  cpu_usage_pct: number;
  ram_total_mb: number;
  ram_used_mb: number;
  ram_free_mb: number;
  disk_total_gb: number;
  disk_used_gb: number;
  disk_free_gb: number;
  active_instances: number;
  total_instances: number;
  uptime_seconds: number;
  lan_ip?: string;
  public_ip?: string;
}

export interface BackupItem {
  id: string;
  filename: string;
  size_bytes: number;
  sha256: string;
}

export interface FileItem {
  name: string;
  relative_path: string;
  is_directory: boolean;
  size_bytes: number;
  modified_epoch_ms: number;
  extension: string;
}

