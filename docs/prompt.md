### receive a json format like this:
{
    type: "switch" \ "move"
    target_index: int 
    move_index : int
}

- example1: 
{
    type: "move"
    target_index: 0
    move_index : 1
}

- example2:
{
    type: "switch"
    target_index: 1
}



put into the queue

### Send a json format
{
    battle_all_info //using index
                    //index -> int PP or HP 
    description: "win" \ "lose" \ "abilities" \ "items"  \ "move side effects" ;
}

- example1:
{
    battle{
        side_a:{
            "current_pokemon_index": NULL,
            pokemons:[
                {
                    name: "pikachu",
                    hp: 100,
                    moves:[
                        {
                            name: "thunderbolt",
                            pp: 15
                            pp_remains: 10
                        },
                        {
                            name: "quick attack",
                            pp: 30
                            pp_remains: 25
                        }
                    ]
                },
                {
                    name: "charizard",
                    hp: 200,
                    moves:[
                        {
                            name: "flamethrower",
                            pp: 15
                            pp_remains: 15
                        },
                        {
                            name: "fly",
                            pp: 20
                            pp_remains: 20
                        }
                    ]
                }
            ]

            in_battle_effects:[
                {
                    name: "paralysis",
                    turns_remains: 2
                }
            ]

            side_b {
                "current_pokemon_index": NULL,
                pokemons:[
                    {
                        name: "blastoise",
                        hp: 150,
                        moves:[
                            {
                                name: "water gun",
                                pp: 25
                                pp_remains: 20
                            },
                            {
                                name: "hydro pump",
                                pp: 5
                                pp_remains: 5
                            }
                        ]
                    },
                    {
                        name: "venusaur",
                        hp: 180,
                        moves:[
                            {
                                name: "razor leaf",
                                pp: 30
                                pp_remains: 30
                            },
                            {
                                name: "solar beam",
                                pp: 10
                                pp_remains: 10
                            }
                        ]
                    }
                ]

                in_battle_effects:[
                    {
                        name: "burn",
                        turns_remains: 3
                    }
                ]
        }
        descriptions:[
            "bistoise used water gun, it caused 30 damage to charizard and charizard's hp is now 170",
            "charizard is burned, it will lose 10 hp for 3 turns"
            "charizard's ability activated, it will increase charizard's attack by 20% for 2 turns"
        ]
        }
    }
}

