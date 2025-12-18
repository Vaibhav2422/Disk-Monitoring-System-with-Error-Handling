"use client"

import useSWR from "swr"
import { Button } from "@/components/ui/button"
import { fetcher } from "@/lib/api"

type LogEntry = string

function asText(entry: LogEntry) {
  return String(entry)
}

export function LogsList() {
  const { data, isLoading, mutate } = useSWR<{ logs: LogEntry[] }>("/disk/logs", fetcher, {
    revalidateOnFocus: false,
  })

  const logs = data?.logs || []

  return (
    <div className="space-y-3">
      <div className="flex items-center justify-between">
        <div className="text-sm text-muted-foreground">{isLoading ? "Loading..." : `${logs.length} log entries`}</div>
        <Button size="sm" variant="outline" onClick={() => mutate()}>
          Refresh
        </Button>
      </div>

      <ul className="space-y-2 text-sm">
        {logs.map((l, i) => (
          <li key={i} className="rounded-md border p-2">
            {asText(l)}
          </li>
        ))}
        {logs.length === 0 && !isLoading && (
          <li className="text-muted-foreground">No logs available. Try performing some actions or refreshing.</li>
        )}
      </ul>
    </div>
  )
}