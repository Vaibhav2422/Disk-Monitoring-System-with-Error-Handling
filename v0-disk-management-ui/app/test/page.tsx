"use client"

import { useEffect, useState } from "react"

export default function TestPage() {
  const [data, setData] = useState<any>(null)
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    const fetchData = async () => {
      try {
        const response = await fetch("/disk/logs")
        const result = await response.json()
        console.log("Raw API response:", result)
        setData(result)
        setLoading(false)
      } catch (err) {
        console.error("Error fetching data:", err)
        setError("Failed to fetch data")
        setLoading(false)
      }
    }

    fetchData()
  }, [])

  if (loading) return <div>Loading...</div>
  if (error) return <div>Error: {error}</div>

  return (
    <div>
      <h1>Test Page</h1>
      <pre>{JSON.stringify(data, null, 2)}</pre>
    </div>
  )
}