cmake_minimum_required(VERSION 3.10)

project(RakNet)

# Force debug-like flags for RakNet in all configurations
# 神秘bug,raknet不能加o2优化
set(CMAKE_CXX_FLAGS_RELEASE "-O0 -g" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-O0 -g" CACHE STRING "" FORCE)

add_definitions(-DRAKSAMP_CLIENT)

# RakNet source files
set(RAKNET_SOURCES
    src/_findfirst.cpp
    src/AsynchronousFileIO.cpp
    src/BitStream.cpp
    src/BitStream_NoTemplate.cpp
    src/CheckSum.cpp
    src/DataBlockEncryptor.cpp
    src/DataCompressor.cpp
    src/DS_ByteQueue.cpp
    src/DS_HuffmanEncodingTree.cpp
    src/DS_Table.cpp
    src/EncodeClassName.cpp
    src/GetTime.cpp
    src/InternalPacketPool.cpp
    src/LinuxStrings.cpp
    src/NetworkTypes.cpp
    src/PluginInterface.cpp
    src/RakClient.cpp
    src/RakNetStatistics.cpp
    src/RakNetworkFactory.cpp
    src/RakPeer.cpp
    src/RakSleep.cpp
    src/Rand.cpp
    src/ReliabilityLayer.cpp
    src/rijndael.cpp
    src/RPCMap.cpp
    src/SHA1.cpp
        src/SimpleMutex.cpp
        src/RakServer.cpp
    src/SocketLayer.cpp
    src/StringCompressor.cpp
    src/StringTable.cpp
    src/SystemAddressList.cpp
    src/TableSerializer.cpp
    src/SAMP/samp_auth.cpp
    src/SAMP/samp_netencr.cpp
    src/SAMP/SAMPRPC.cpp
)

# RakNet header files
set(RAKNET_HEADERS
    src/_findfirst.h
    src/AsynchronousFileIO.h
    src/AutopatcherPatchContext.h
    src/AutopatcherRepositoryInterface.h
    src/BigTypes.h
    src/BitStream.h
    src/BitStream_NoTemplate.h
    src/CheckSum.h
    src/ClientContextStruct.h
    src/DataBlockEncryptor.h
    src/DataCompressor.h
    src/DS_BinarySearchTree.h
    src/DS_BPlusTree.h
    src/DS_ByteQueue.h
    src/DS_Heap.h
    src/DS_HuffmanEncodingTree.h
    src/DS_HuffmanEncodingTreeFactory.h
    src/DS_HuffmanEncodingTreeNode.h
    src/DS_LinkedList.h
    src/DS_List.h
    src/DS_Map.h
    src/DS_MemoryPool.h
    src/DS_OrderedChannelHeap.h
    src/DS_OrderedList.h
    src/DS_Queue.h
    src/DS_QueueLinkedList.h
    src/DS_RangeList.h
    src/DS_Table.h
    src/DS_Tree.h
    src/DS_WeightedGraph.h
    src/EncodeClassName.h
    src/Export.h
    src/GetTime.h
    src/InternalPacket.h
        src/RakServer.h
    src/InternalPacketPool.h
    src/LinuxStrings.h
    src/MTUSize.h
    src/multiplayer.h
    src/NetworkTypes.h
    src/PacketEnumerations.h
    src/PacketPool.h
    src/PacketPriority.h
    src/PluginInterface.h
    src/RakAssert.h
    src/RakClient.h
    src/RakClientInterface.h
    src/RakNetDefines.h
    src/RakNetStatistics.h
    src/RakNetworkFactory.h
    src/RakPeer.h
    src/RakPeerInterface.h
    src/RakSleep.h
    src/Rand.h
    src/ReliabilityLayer.h
    src/Replica.h
    src/ReplicaEnums.h
    src/rijndael-boxes.h
    src/rijndael.h
    src/RouterInterface.h
    src/RPCMap.h
    src/RPCNode.h
    src/RSACrypt.h
    src/SHA1.h
    src/SimpleMutex.h
    src/SingleProducerConsumer.h
    src/SocketLayer.h
    src/StringCompressor.h
    src/StringTable.h
    src/SystemAddressList.h
    src/TableSerializer.h
    src/ThreadPool.h
    src/Types.h
    src/SAMP/samp_auth.h
    src/SAMP/samp_netencr.h
    src/SAMP/SAMP_VER.h
    src/SAMP/SAMPRPC.h
    src/SAMP/WinTypes.h
)
add_library(RakNet STATIC ${RAKNET_SOURCES})

target_include_directories(RakNet PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(RakNet ${CMAKE_THREAD_LIBS_INIT})
endif()

target_compile_definitions(RakNet PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
)

