"use client"

import useSWR from "swr"
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card"
import { Button } from "@/components/ui/button"
import { fetcher } from "@/lib/api"

type Stats = {
  total: number
  used: number
  free: number
  fragmentationPercent: number
  badSectors: number
}

export function StatsCard() {
  const { data, isLoading, mutate, error } = useSWR<Stats>("/disk/stats", fetcher, {
    revalidateOnFocus: false,
  })

  if (isLoading) {
    return (
      <Card>
        <CardHeader className="flex items-center justify-between flex-row">
          <CardTitle>Stats</CardTitle>
          <Button size="sm" variant="outline" onClick={() => mutate()}>
            Refresh
          </Button>
        </CardHeader>
        <CardContent>
          <div className="text-sm text-muted-foreground">Loading...</div>
        </CardContent>
      </Card>
    )
  }

  if (error) {
    return (
      <Card>
        <CardHeader className="flex items-center justify-between flex-row">
          <CardTitle>Stats</CardTitle>
          <Button size="sm" variant="outline" onClick={() => mutate()}>
            Refresh
          </Button>
        </CardHeader>
        <CardContent>
          <div className="text-sm text-red-500">Failed to load stats</div>
        </CardContent>
      </Card>
    )
  }

  if (!data) {
    return (
      <Card>
        <CardHeader className="flex items-center justify-between flex-row">
          <CardTitle>Stats</CardTitle>
          <Button size="sm" variant="outline" onClick={() => mutate()}>
            Refresh
          </Button>
        </CardHeader>
        <CardContent>
          <div className="text-sm text-muted-foreground">No data available</div>
        </CardContent>
      </Card>
    )
  }

  const fragPercent = `${data.fragmentationPercent.toFixed(2)}%`

  return (
    <Card>
      <CardHeader className="flex items-center justify-between flex-row">
        <CardTitle>Stats</CardTitle>
        <Button size="sm" variant="outline" onClick={() => mutate()}>
          Refresh
        </Button>
      </CardHeader>
      <CardContent className="grid grid-cols-2 gap-3 text-sm">
        <div>
          <div className="text-muted-foreground">Total</div>
          <div className="font-medium">{data.total}</div>
        </div>
        <div>
          <div className="text-muted-foreground">Used</div>
          <div className="font-medium">{data.used}</div>
        </div>
        <div>
          <div className="text-muted-foreground">Free</div>
          <div className="font-medium">{data.free}</div>
        </div>
        <div>
          <div className="text-muted-foreground">Bad Sectors</div>
          <div className="font-medium">{data.badSectors}</div>
        </div>
        <div className="col-span-2">
          <div className="text-muted-foreground">Fragmentation</div>
          <div className="font-medium">{fragPercent}</div>
        </div>
      </CardContent>
    </Card>
  )
}