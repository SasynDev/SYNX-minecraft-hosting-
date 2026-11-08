import { InstanceSummary, InstanceDetail, HostMetrics, BackupItem } from './types';

const API_BASE = '/api/v1';
const AUTH_TOKEN = 'synx_agent_dev_token_default_secret_998244353';

async function request<T>(endpoint: string, options: RequestInit = {}): Promise<T> {
  const headers = {
    'Content-Type': 'application/json',
    'X-Synx-Agent-Token': AUTH_TOKEN,
    ...options.headers,
  };

  const response = await fetch(`${API_BASE}${endpoint}`, {
    ...options,
    headers,
  });

  if (!response.ok) {
    const errorText = await response.text();
    let errorMsg = `Request failed: ${response.status} ${response.statusText}`;
    try {
      const errJson = JSON.parse(errorText);
      if (errJson.error) errorMsg = errJson.error;
    } catch {
      if (errorText) errorMsg = errorText;
    }
    throw new Error(errorMsg);
  }

  return response.json();
}

export const api = {
  getHealth: () => request<{ status: string; version: string; brand: string }>('/health'),
  
  getHostMetrics: () => request<HostMetrics>('/host/metrics'),
  
  getInstances: () => request<InstanceSummary[]>('/instances'),
  
  getInstance: (id: string) => request<InstanceDetail>(`/instances/${id}`),
  
  createInstance: (data: {
    name: string;
    server_type: string;
    mc_version: string;
    ram_limit_mb: number;
    cpu_limit_pct: number;
    cracked_enabled: boolean;
    crossplay_enabled?: boolean;
    port?: number;
  }) => request<{ success: boolean; id: string; identifier: string; port: number }>('/instances', {
    method: 'POST',
    body: JSON.stringify(data),
  }),

  sendPowerAction: (id: string, action: 'start' | 'stop' | 'restart' | 'kill') =>
    request<{ success: boolean; action: string }>(`/instances/${id}/power`, {
      method: 'POST',
      body: JSON.stringify({ action }),
    }),

  sendCommand: (id: string, command: string) =>
    request<{ success: boolean }>(`/instances/${id}/command`, {
      method: 'POST',
      body: JSON.stringify({ command }),
    }),

  getConsoleLines: (id: string) =>
    request<{ lines: string[] }>(`/instances/${id}/console`),

  getProperties: (id: string) =>
    request<Record<string, string>>(`/instances/${id}/properties`),

  updateProperties: (id: string, properties: Record<string, string>) =>
    request<{ success: boolean }>(`/instances/${id}/properties`, {
      method: 'POST',
      body: JSON.stringify(properties),
    }),

  setCrackedMode: (id: string, enabled: boolean) =>
    request<{ success: boolean; cracked_enabled: boolean }>(`/instances/${id}/cracked`, {
      method: 'POST',
      body: JSON.stringify({ enabled }),
    }),

  createBackup: (id: string) =>
    request<BackupItem>(`/instances/${id}/backups`, {
      method: 'POST',
    }),
};

