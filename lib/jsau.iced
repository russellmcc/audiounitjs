fs = require 'fs'
path = require 'path'
child_process = require 'child_process'

# array of replacements strings
keys =
  '#PROJNAME' : 'project'
  '#AUTYPE' : 'type'
  '#TYPECODE' : 'plugin_id'
  '#MANUCODE' : 'manufacturer_id'
  '#NAME' : 'name'
  '#DESCRIPTION' : 'description'
  '#COMPANY_UNDERSCORED' : 'company_underscore'
  '#COMPANY' : 'company'


# help for each required key (keep synced with
key_help =
  'project' : "the name of the project file - users won't see this, but you will.  Must not contain spaces."
  'type' : "the AU type.  must be aufx for now."
  'plugin_id' : "the four-character plugin id."
  'manufacturer_id' : "the four-character manufacturer id."
  'name' : "the user-visible name of the plug in"
  'description' : "the user-visible description"
  'company' : "the user-visible company."

transform_file = (src, project, target) ->

  srcStream = fs.createReadStream src
  targetStream = fs.createWriteStream target
  srcStream.pipe targetStream

  await srcStream.on 'end', defer err

  srcStream.destroy()
  targetStream.destroy()

  # replace each key with each replacement using sed
  args = ['-i', '']
  for key, rep of keys
    args = args.concat ['-e', "s/#{key}/#{project[rep]}/g"]
  args = args.concat [target]

  proc = child_process.spawn 'sed', args, {'stdio' : ['ignore', 'ignore', 2]}
  await proc.on 'exit', defer code


transform_dir = (dir, project, target) ->

  # create the destination
  await fs.mkdir target, defer err
  throw err if err?

  # read each file from the source.
  await fs.readdir dir, defer err, files
  throw err if err?

  # now tranform each file and subdirectory.
  for file in files
    src_path = path.join dir, file
    target_path = path.join target, file

    # replace all keys
    for key, rep of keys
      target_path = target_path.replace key, project[rep]

    await fs.stat src_path, defer err, stats
    throw err if err?

    # if it's a file, transform the file
    if stats.isFile()
      transform_file src_path, project, target_path
    else if stats.isDirectory()
      transform_dir src_path, project, target_path

module.exports = (json_file) ->

  project = JSON.parse fs.readFileSync json_file

  throw "Couldn't parse project file #{json_file}" if not project?

  project.company_underscore = project.company?.replace /\s/g, '_'

  for own key, help of key_help
    throw "missing project property '#{key}'.  This should be set to #{help}" if not project[key]?

  await fs.realpath (path.join __dirname, '..', 'Template'), defer err, fullpath
  throw err if err?
  target = (path.join process.cwd(), project.project)

  transform_dir fullpath, project, target