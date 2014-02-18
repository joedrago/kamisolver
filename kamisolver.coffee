fs = require 'fs'

quit = (reason) ->
  console.log "ERROR: #{reason}"
  process.exit()

class Node
  constructor: (@solver, @id, @x, @y, @color) ->
    @connections = {}

  connect: (node) ->
    if not @connections[node.id]
      @connections[node.id] = 1
      node.connect(this)

  disconnect: (node) ->
    if @connections[node.id]
      delete @connections[node.id]
      node.disconnect(this)

  consume: (node) ->
    conns = (c for c of node.connections)
    for c in conns
      node.disconnect(@solver.nodes[c])
      @connect(@solver.nodes[c])

class KamiSolver
  constructor: (@filename) ->
    @nodes = {}
    currentID = 0

    # Read in lines, do some sanity checking
    lines = (line for line in fs.readFileSync(@filename, { encoding: 'utf-8' }).split(/\r|\n/) when line.length > 0)
    if lines.length != 10
      quit "#{@filename} has #{lines.length} lines, was expecting 10"
    for line,i in lines
      if line.length != 16
        quit "Line ##{i+1} has #{line.length} chars, was expecting 16"
      if not /^[0-9]+$/.test(line)
        quit "Line ##{i+1} has non-numerics"

    # Turn the raw data into a crappy graph
    lineNodes = null
    for line,lineNo in lines
      prevLineNodes = lineNodes
      lineNodes = []
      prevNode = null
      for i in [0..15]
        color = parseInt(line[i])
        node = new Node(this, currentID, i, lineNo, color)
        currentID++
        @nodes[node.id] = node
        lineNodes.push node
        if prevNode
          prevNode.connect(node)
        if prevLineNodes
          prevLineNodes[i].connect(node)
        prevNode = node

    # ... then make the graph not crappy
    @coalesceNodes()

  coalesceNodes: ->
    idList = (id for id of @nodes)
    for id in idList
      node = @nodes[id]
      continue if not node?

      loop
        conns = (c for c of node.connections)
        consumedOne = false
        for connID in conns
          if (node.id < connID) and (node.color == @nodes[connID].color)
            node.consume @nodes[connID]
            delete @nodes[connID]
            consumedOne = true

        break if not consumedOne

  dump: (nodes) ->
    # Pipe this output to: circo -Tpng -o out.png
    nodes ?= @nodes
    console.log "graph G {"
    console.log "overlap=prism;"

    idList = (id for id of @nodes)
    for id in idList
      node = @nodes[id]
      #console.log "N#{node.id} [label=\"N#{node.id} color #{node.color} (#{node.x}, #{node.y})\" ];"
      console.log "N#{node.id} [label=\"#{node.color} (#{node.x}, #{node.y})\" ];"
      for c of node.connections
        if c > node.id
          console.log "N#{node.id} -- N#{c};"

    console.log "}"

  solve: ->

main = ->
  solver = new KamiSolver('A9.txt')
  solver.dump()

main()
