fs = require 'fs'

clone = (obj) ->
  if not obj? or typeof obj isnt 'object'
    return obj

  if obj instanceof Date
    return new Date(obj.getTime())

  if obj instanceof RegExp
    flags = ''
    flags += 'g' if obj.global?
    flags += 'i' if obj.ignoreCase?
    flags += 'm' if obj.multiline?
    flags += 'y' if obj.sticky?
    return new RegExp(obj.source, flags)

  newInstance = new obj.constructor()

  for key of obj
    newInstance[key] = clone obj[key]

  return newInstance

# colorToLabel = (color) ->
#   switch color
#     when 'R' then 'Red'
#     when 'G' then 'Green'
#     when 'B' then 'Blue'
#     when 'K' then 'Black'
#     when 'O' then 'Orange'
#     when 'W' then 'White'
#     when 'N' then 'Brown'
#     else "Unknown"

colorToHex = (color) ->
  switch color
    when 'R' then ['#CA3736', '#000000']
    when 'G' then ['#52C1A5', '#000000']
    when 'B' then ['#0000ff', '#000000']
    when 'K' then ['#1E140F', '#ffffff']
    when 'O' then ['#ED6C3A', '#000000']
    when 'W' then ['#ffffff', '#000000']
    when 'N' then ['#9B7065', '#ffffff']
    else "Unknown"

quit = (reason) ->
  console.log "ERROR: #{reason}"
  process.exit()

class Node
  constructor: (@id, @x, @y, @color) ->
    @connections = {}

  connect: (node) ->
    if not @connections[node.id]
      @connections[node.id] = 1
      node.connect(this)

  disconnect: (node) ->
    if @connections[node.id]
      delete @connections[node.id]
      node.disconnect(this)

  consume: (solver, node) ->
    conns = (c for c of node.connections)
    for c in conns
      node.disconnect(solver.nodes[c])
      @connect(solver.nodes[c])

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
      if not /^[RGBKOWN]+$/.test(line)
        quit "Line ##{i+1} has invalid letters"

    # Turn the raw data into a crappy graph
    lineNodes = null
    for line,lineNo in lines
      prevLineNodes = lineNodes
      lineNodes = []
      prevNode = null
      for i in [0..15]
        color = line[i]
        node = new Node(currentID, i, lineNo, color)
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
            node.consume this, @nodes[connID]
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
      colors = colorToHex(node.color)
      console.log "N#{node.id} [style=filled; fillcolor=\"#{colors[0]}\"; fontcolor=\"#{colors[1]}\"; label=\"#{node.x}, #{node.y}\" ];"
      for c of node.connections
        if c > node.id
          console.log "N#{node.id} -- N#{c};"

    console.log "}"

  solve: (remainingMoves, nodes) ->
    nodes ?= clone(nodes)

    console.log "solving #{nodes}"

main = ->
  solver = new KamiSolver('A9.txt')
  solver.solve(4)

main()
