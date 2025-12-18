export const API_BASE_URL = 'http://localhost:3003';

export function buildUrl(path: string): string {
  // Remove leading slash if present to avoid double slashes
  const cleanPath = path.startsWith('/') ? path.slice(1) : path;
  return `${API_BASE_URL}/${cleanPath}`;
}

export const fetcher = async (url: string) => {
  const fullUrl = url.startsWith('http') ? url : buildUrl(url);
  const res = await fetch(fullUrl);
  if (!res.ok) {
    throw new Error(`HTTP error! status: ${res.status}`);
  }
  const data = await res.json();
  // Always return the data property if it exists, otherwise return the whole response
  if (data && typeof data === 'object' && 'data' in data) {
    return data.data;
  }
  return data;
}

export const postJSON = async (url: string, body: any) => {
  const fullUrl = url.startsWith('http') ? url : buildUrl(url);
  const res = await fetch(fullUrl, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify(body),
  });
  return res;
}

export const postEmpty = async (url: string) => {
  const fullUrl = url.startsWith('http') ? url : buildUrl(url);
  const res = await fetch(fullUrl, {
    method: 'POST',
  });
  return res;
}

export const deleteRequest = async (url: string) => {
  const fullUrl = url.startsWith('http') ? url : buildUrl(url);
  const res = await fetch(fullUrl, {
    method: 'DELETE',
  });
  return res;
}