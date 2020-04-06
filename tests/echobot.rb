#!/usr/bin/env ruby
require 'discordrb'

bot = Discordrb::Bot.new token: File.read("#{$0.split('.')[0]}.token")
puts "This bot's invite URL is #{bot.invite_url}."
puts 'Click on it to invite it to your server.'

bot.message(start_with: '$') do |e|
  unless ARGV.include? 'whatver'
    e.respond "!#{e.message.to_s[1..-1]}"
  end
end

bot.run
