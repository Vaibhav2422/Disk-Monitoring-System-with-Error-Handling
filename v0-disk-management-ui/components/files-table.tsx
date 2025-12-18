"use client"

import useSWR from "swr"
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from "@/components/ui/table"
import { Button } from "@/components/ui/button"
import { fetcher } from "@/lib/api"

type FileRow = {
  id: string | number
  size?: number
  blocks?: number
  createdAt?: string
  [key: string]: any
}

export function FilesTable() {
  const { data, isLoading, mutate } = useSWR<{ files: FileRow[] }>("/disk/files", fetcher, {
    revalidateOnFocus: false,
  })

  const rows: FileRow[] = data?.files || []

  return (
    <div className="space-y-3">
      <div className="flex items-center justify-between">
        <div className="text-sm text-muted-foreground">{isLoading ? "Loading..." : `${rows.length} files`}</div>
        <Button size="sm" variant="outline" onClick={() => mutate()}>
          Refresh
        </Button>
      </div>

      <div className="rounded-md border overflow-x-auto">
        <Table>
          <TableHeader>
            <TableRow>
              <TableHead>ID</TableHead>
              <TableHead>Size</TableHead>
              <TableHead>Blocks</TableHead>
              <TableHead>Created</TableHead>
            </TableRow>
          </TableHeader>
          <TableBody>
            {rows.map((f) => (
              <TableRow key={String(f.id)}>
                <TableCell className="font-medium">{String(f.id)}</TableCell>
                <TableCell>{f.size ?? "-"}</TableCell>
                <TableCell>{f.blocks ?? "-"}</TableCell>
                <TableCell className="text-muted-foreground">{f.createdAt ?? "-"}</TableCell>
              </TableRow>
            ))}
            {rows.length === 0 && !isLoading && (
              <TableRow>
                <TableCell colSpan={4} className="text-center text-muted-foreground">
                  No files
                </TableCell>
              </TableRow>
            )}
          </TableBody>
        </Table>
      </div>
    </div>
  )
}