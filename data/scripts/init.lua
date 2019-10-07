local engine = require('engine')
local guppy = require('archetypes.guppy')
local spawn_food = require('archetypes.food')

config = {
}

game_state = {
    money = 50,
    shop_items = {
        {
            name = 'food',
            cost = 1,
            action = function ()
                spawn_food({ x = -8.4 + math.random() * 16.8 })
                spawn_food({ x = -8.4 + math.random() * 16.8 })
                spawn_food({ x = -8.4 + math.random() * 16.8 })
                spawn_food({ x = -8.4 + math.random() * 16.8 })
                spawn_food({ x = -8.4 + math.random() * 16.8 })
                play_sfx('splash')
            end
        },
        {
            name = 'guppy',
            cost = 1,
            action = function () guppy({ sprite = 0, growthrate = 30, bubblemin = 15, grownbubble = 2 }) play_sfx('splash') end
        },
        {
            name = 'tang',
            cost = 10,
            action = function () guppy({ sprite = 2, growthrate = 60, bubblemin = 10, grownbubble = 2 }) play_sfx('splash') end
        },
        {
            name = 'eel',
            cost = 50,
            action = function () guppy({ sprite = 4, growthrate = 20, bubblemin = 20, grownbubble = 3 }) play_sfx('splash') end
        },
        {
            name = 'angel',
            cost = 100,
            action = function () guppy({ sprite = 6, growthrate = 90, bubblemin = 15, grownbubble = 4 }) play_sfx('splash') end
        },
        {
            name = 'pinku',
            cost = 250,
            action = function () guppy({ sprite = 8, growthrate = 5*30, bubblemin = 0, grownbubble = 5 }) play_sfx('splash') end
        }
    }
}

gui_state = {
    fps = 0,
    debug_strings = {},
    debug_vals = {},
    game_state = game_state
}

guppy({ sprite = 0, growthrate = 30, bubblemin = 15, grownbubble = 2 })
play_sfx('splash')

print('init done')
