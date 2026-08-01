package main

import (
	"fmt"
	"net/http"
	"runtime"
)

const responseJSON = `{"message":"Hello, World!"}`

func handler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	w.Header().Set("Content-Length", "27")
	w.Write([]byte(responseJSON))
}

func main() {
	runtime.GOMAXPROCS(runtime.NumCPU())
	fmt.Println("Go Server listening on port 8081...")
	http.HandleFunc("/json", handler)
	if err := http.ListenAndServe(":8081", nil); err != nil {
		panic(err)
	}
}
