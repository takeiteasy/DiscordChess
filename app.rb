#!/usr/bin/env ruby
%w(discordrb redis thread securerandom).each { |r| require r }

Thread.abort_on_exception = true
$stdout.sync = true

$bot = Discordrb::Commands::CommandBot.new token: File.read('token'), prefix: '!'
puts "This bot's invite URL is #{$bot.invite_url}."
puts 'Click on it to invite it to your server.'
# TODO: Config parser for bot token and Redis information
$db  = Redis.new

# Basic thread-safe hash wrapper
class ThreadHash
  def initialize
    @mutex = Mutex.new
    @hash  = {}
  end

  def has? id
    @mutex.synchronize do
      @hash.key? id
    end
  end

  def push id, data
    false if self.has? id
    @mutex.synchronize do
      @hash[id] = data
    end
    true
  end

  def pop id
    false unless self.has? id
    @mutex.synchronize do
      @hash.delete id
    end
    true
  end

  def use &block
    @mutex.synchronize do
      yield @hash
    end
  end

  def to_s
    @mutex.synchronize do
      @hash.to_s
    end
  end

  def length
    @mutex.synchronize do
      @hash.length
    end
  end
end

# Class to handle user data and wrapping around discordrb & redis
class User
  attr_accessor :id, :nick, :discr

  def set id=nil, nick=nil, discriminator=nil
    case id
    when Fixnum, String
      @id = id
      if nick and discriminator
        @nick  = nick
        @discr = discriminator
      else
        self.update
      end
    when Discordrb::Member, Discordrb::User
      @id    = id.id
      @nick  = id.username
      @discr = id.discriminator
    else
      @id    = nil
      @nick  = nil 
      @discr = nil 
    end
  end

  def initialize id=nil, nick=nil, discriminator=nil
    self.set id, nick, discriminator
  end

  def to_s
    "#{@nick}##{@discr}"
  end

  def dbid
    "user:#{@id}"
  end

  def is_set?
    not (@id.nil? and @nick.nil? and @discr.nil?)
  end

  def is_valid?
    self.is_set? and not $bot.user(@id).nil? and self.exists?
  end

  def resolve a
    u = nil
    case a
    when /^\d+$/
      u = $bot.user a
    when /^.*#\d+$/
      b = a.rpartition '#'
      u = $bot.find_user b.first, b.last
    else
      false
    end

    self.set u unless u.nil?
    not u.nil?
  end

  def update
    false unless @id
    u = $bot.user @id
    false unless u
    self.set u
    true
  end

  def exists?
    $db.exists self.dbid
    # TODO: Print to a log file instead of stdout
  end

  def pm msg
    return unless self.is_valid?
    begin
      u = $bot.user @id
      u.pm msg
    rescue
      puts "PM: #{msg}"
    end
  end
end

$mm_list = ThreadHash.new # Matchmaking list
$pc_list = ThreadHash.new # Personal challenge list
$ag_list = ThreadHash.new # Active games list

# Printed after bot commands. Tells you when, where and who
def whereami user, from
  # TODO: Print to a log file instead of stdout
  puts "[#{Time.now}] #{from}: #{user.id} (#{user.username}##{user.discriminator})"
end

# Register discord account to the bot
$bot.command :register do |e|
  whereami e.user, "REGISTER"
  user = "user:#{e.user.id}"
  if $db.exists user 
    e.user.pm 'You already registered! Type **!help** for a list of commands'
  else
    $db.hmset user, :elo, 1000, :history, "", :age, Time.now.getutc.to_i, :wins, 0, :loses, 0 
    e.user.pm 'You are now registered. Welcome! Type **!help** for a list of commands'
  end
end

# Challenge another player
$bot.command :challenge do |e, *args|
  whereami e.user, "CHALLENGE"
  challenger = User.new e.user
  "**Sorry!** You aren't registered. Type **!register** to begin" unless challenger.is_valid?

  a = args.join(' ')
  opponent = User.new a 
  "**Sorry!** I can't find user **'#{a}'**" unless opponent.is_valid?

  if challenger.id == opponent.id
    challenger.pm "**Sorry!** You can't challenge yourself" 
    return
  end

  gid = SecureRandom.uuid
  send_challenge = lambda do
    challenger.pm "You have challenged **#{opponent}!**. Type **!cancel #{gid}** to cancel the challenge"
    opponent.pm "You have been challenged by **#{challenger}!**. Type **!accept #{gid}** to accept the challenge"
  end

  # TODO: Check if challenger or opponent is already in a game
  if $pc_list.has? challenger.id
    $pc_list.use do |h|
      y = h[challenger.id].detect do |x|
        x[:uid] == opponent.id
      end
      unless y
        h[challenger.id].push({ :uid => opponent.id, :gid => gid, :age => Time.now.getutc.to_i })
        send_challenge.call
      else
        "You have already challenged **#{opponent.to_s}**! Awaiting their response..."
      end
    end
  elsif $pc_list.has? opponent.id
    $pc_list.use do |h|
      y = h[opponent.id].detect do |x|
        x[:uid] == challenger.id
      end
      unless y
        h[challenger.id].push({ :uid => opponent.id, :gid => gid, :age => Time.now.getutc.to_i})
        send_challenge.call
      else
        # TODO: Start game here
        "Starting match between **#{challenger}** and **#{opponent}**! (ID: #{gid})"
      end
    end
  else
    $pc_list.push challenger.id, [{ :uid => opponent.id, :gid => gid, :age => Time.now.getutc.to_i }]
    send_challenge.call
  end
end

def find_challenge uid, gid
  $pc_list.use do |h|
    h.each do |k, v|
      y = h[k].detect do |x|
        x[:gid] == gid and x[:uid] == uid
      end
      return y if y.merge!(cid: k)
    end
  end
  nil
end


def remove_challenge uid, gid
  $pc_list.use do |h|
    h.each do |k, v|
      h[k].delete_if do |x|
        x[:gid] == gid and x[:uid] == uid
      end
      h.delete k if h[k].empty?
    end
  end
end

# Accept a challenge/matchmaking game
$bot.command :accept do |e, *args|
  whereami e.user, "ACCEPT"
  gid = args.join(' ')
  user = User.new e.user
  if gid.empty?
    # TODO: Accept matchmaking game
    return
  else
    g = find_challenge user.id, gid
    if g
      challenger = User.new g[:cid]
      user.pm "You have accepted **#{challenger}**'s challenge! Game is beginning!"
      challenger.pm "**#{user}** has accepted your challenge! Game is beginning!"
      remove_challenge user.id, gid
      # TODO: Start game here
      "Starting match between **#{challenger}** and **#{user}**! (ID: #{gid})"
    else
      "**Sorry!** Can't find game '#{gid}'. It may have been declined or timed out"
    end
  end
end

# Decline a challenge/matchmaking game
$bot.command :decline do |e, *args|
  whereami e.user, "DECLINE"
  gid = args.join(' ')
  user = User.new e.user
  if gid.empty?
    # TODO: Decline matchmaking game
    return
  else
    g = find_challenge user, gid
    if g
      challenger = User.new g[:cid]
      user.pm "You have declined **#{challenger}**'s challenge!"
      challenger.pm "**#{user}** has declined your challenge"
      remove_challenge user.id, gid
    else
      "**Sorry!** Can't find game '#{gid}'. It may have already been declined or timed out"
    end
  end
end

# Matchmaking for match w/ similar elo
$bot.command :search do |e|
  whereami e.user, "SEARCH"
end

# Cancel challenge/matchmaking
$bot.command :cancel do |e, *args|
  whereami e.user, "CANCEL"
  gid = args.join(' ')
  user = User.new e.user
  if gid.empty?
    # TODO: Cancel matchmaking search 
    return
  else
    $pc_list.use do |h|
      return "**Sorry!** Can't find game '#{gid}'. It may have already been declined or timed out" unless h.has_key? user.id
      y = h[user.id].detect do |x|
        x[:gid] == gid
      end
      if y
        opponent = User.new y[:uid]
        user.pm "You have cancelled your challenge to **#{opponent}**!"
        opponent.pm "**#{user}** has cancelled their challenge to you! (#{gid})"
        h[user.id].delete_if do |x|
          x[:gid] == gid and x[:uid] == opponent.id
        end
        h.delete user.id if h[user.id].empty?
        return
      else
        return "**Sorry!** Can't find game '#{gid}'. It may have already been declined or timed out"
      end
    end
  end
end

# Get a player's elo rating
$bot.command :elo do |e, *args|
  whereami e.user, "ELO"
  a = (args.empty? ? e.user.id.to_s : args.join(' '))
  user = User.new a
  if user.is_valid?
    "**#{user.to_s}** (#{user.id}) has **#{$db.hget(user.dbid, :elo)}** elo"
  else
    "**Sorry!** I can't find user **'#{a}'**"
  end
end

# Get a player's information
$bot.command :info do |e, *args|
  whereami e.user, "INFO"
  a = (args.empty? ? e.user.id.to_s : args.join(' '))
  user = User.new a
  if user.is_valid?
    "**#{user.to_s}** (#{user.id}) has **#{$db.hget(user.dbid, :elo)}** elo"
  else
    "**Sorry!** I can't find user **'#{a}'**"
  end
end

# Concede the current game
$bot.command :concede do |e|
  whereami e.user, "CONCEDE"
end

# Make a move
$bot.command :move do |e, *args|
  whereami e.user, "MOVE"
end

# Test function that evaluates Ruby code. Only works if you're me. Which you're not. So don't worry about this. Move along.
$bot.command(:eval, help_available: false) do |e, *code|
  break unless e.user.id == 158642095291105280
  begin
    eval code.join(' ')
  rescue Exception => err 
    "**ERROR**: #{err}"
  end
end

# Print help
$bot.command :help do |e, *args|
  "help me help you"
end

# Thread to loop through challenge list every minute and delete any older than 10 minutes old.
pc_list_timeout = Thread.new do
  loop do
    $pc_list.use do |h|
      h.each do |k, v|
        next if not v or v.empty?
        challenger = User.new k
        vv, v = v.partition do |x|
          Time.now.getutc.to_i - x[:age] > 600
        end
        next if not vv or vv.empty?

        vv.each do |x|
          opponent = User.new x[:uid]
          challenger.pm "**Sorry!** Your challenge to **#{opponent}** has timed out! (ID: #{x[:gid]})"
        end

        if v.empty?
          h.delete k
        else
          h[k] = v
        end
      end
    end
    sleep 60
  end
end

$bot.run
