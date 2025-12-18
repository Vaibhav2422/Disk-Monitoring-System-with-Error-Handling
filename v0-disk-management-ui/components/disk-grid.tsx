"use client"

import useSWR from "swr"
import { Button } from "@/components/ui/button"
import { cn } from "@/lib/utils"
import { fetcher } from "@/lib/api"

type RawSector = any
type StateResponse = { blocks: RawSector[]; width?: number } | RawSector[]

function normalizeStatus(s: RawSector): "free" | "allocated" | "bad" {
  if (typeof s === "number") {
    if (s < 0) return "bad"
    if (s > 0) return "allocated"
    return "free"
  }
  if (typeof s === "string") {
    const v = s.toLowerCase()
    if (v.includes("bad")) return "bad"
    if (v.includes("alloc")) return "allocated"
    if (v.includes("file")) return "allocated"
    return "free"
  }
  if (s && typeof s === "object") {
    const v = s.status ?? s.state ?? s.type ?? "free"
    return normalizeStatus(v)
  }
  return "free"
}

export function DiskGrid() {
  const { data, isLoading, mutate } = useSWR<StateResponse>("/disk/state", fetcher, {
    revalidateOnFocus: false,
  })

  // Handle the case where data is an object with a blocks property
  const sectors: RawSector[] = Array.isArray(data) ? data : (data?.blocks ?? [])
  const total = sectors.length
  // Create a 32x32 grid layout
  const width = 32
  const rows = Math.ceil(total / width)

  return (
    <div className="space-y-3">
      <div className="flex items-center gap-2">
        <div className="text-sm text-muted-foreground">
          {isLoading ? "Loading..." : `${total} sectors · grid ${width}×${rows}`}
        </div>
        <Button size="sm" variant="outline" onClick={() => mutate()}>
          Refresh
        </Button>
      </div>

      <div
        className="grid gap-[2px] border rounded-md p-2"
        style={{
          gridTemplateColumns: `repeat(${width}, minmax(0, 1fr))`,
          backgroundColor: "var(--color-border)",
        }}
        aria-label="Disk Grid"
        role="grid"
      >
        {sectors.map((s, i) => {
          const status = normalizeStatus(s)
          return (
            <div
              key={i}
              role="gridcell"
              aria-label={`sector ${i + 1} ${status}`}
              className={cn(
                "disk-cell w-4 h-4 md:w-5 md:h-5 lg:w-6 lg:h-6 rounded-sm",
                "border border-border",
                status === "free" && "disk-free",
                status === "allocated" && "disk-allocated",
                status === "bad" && "disk-bad",
              )}
              title={`#${i + 1} - ${status}`}
            />
          )
        })}
      </div>

      <div className="flex items-center gap-3 text-sm">
        <span className="inline-flex items-center gap-1">
          <span className="w-3 h-3 rounded-sm disk-free border border-border" />
          free
        </span>
        <span className="inline-flex items-center gap-1">
          <span className="w-3 h-3 rounded-sm disk-allocated border border-border" />
          allocated
        </span>
        <span className="inline-flex items-center gap-1">
          <span className="w-3 h-3 rounded-sm disk-bad border border-border" />
          bad sector
        </span>
      </div>
    </div>
  )
}