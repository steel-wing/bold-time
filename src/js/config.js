module.exports = [
    {
        "type": "heading",
        "defaultValue": "Bold Time Settings"
    },
    {
        "type": "section",
        "items": [
            {
                "type": "heading",
                "defaultValue": "Colors",
            },
            {
                "type": "color",
                "messageKey": "background_color",
                "defaultValue": "0x000000",
                "allowGray": true,
                "label": "Background Color",
            },  
            {
                "type": "color",
                "messageKey": "hour_one_color",
                "defaultValue": "0xFFFFFF",
                "allowGray": true,
                "label": "First Hour Color",
            },            
            {
                "type": "color",
                "messageKey": "hour_two_color",
                "defaultValue": "0xCCCCCC",
                "allowGray": true,
                "label": "Second Hour Color",
            },
            {
                "type": "color",
                "messageKey": "minute_one_color",
                "defaultValue": "0xCCCCCC",
                "allowGray": true,
                "label": "First Minute Color",
            },
            {
                "type": "color",
                "messageKey": "minute_two_color",
                "defaultValue": "0xFFFFFF",
                "allowGray": true,
                "label": "Second Minute Color",
            },  
        ],
    },
    {
        "type": "section",
        "items": [
            {
                "type": "heading",
                "defaultValue": "Spacing",
            },
            {
                "type": "text",
                "defaultValue": "These control the outer and inner spacings.\nSmaller values are recommended, but larger values can be cute."
            },
            {
                "type": "slider",
                "messageKey": "border_thickness",
                "defaultValue": "2",
                "max": 40,
                "label": "Border Thickness",
            },
            {
                "type": "slider",
                "messageKey": "gap_thickness",
                "defaultValue": "2",
                "max": 40,
                "label": "Gap Thickness",
            },
        ],
    },
    {
        "type": "section",
        "items": [
            {
                "type": "heading",
                "defaultValue": "Alternate Design Toggles",
            },
            {
                "type": "text",
                "defaultValue": "These are some alternate versions of the font.\nJust a stylistic choice."
            },
            {
                "type": "toggle",
                "messageKey": "six_tail",
                "defaultValue": true,
                "label": "Tail Above 6",
            },
            {
                "type": "toggle",
                "messageKey": "seven_tail",
                "defaultValue": false,
                "label": "Tail Left of 7",
            },
            {
                "type": "toggle",
                "messageKey": "nine_tail",
                "defaultValue": true,
                "label": "Tail Below 9",
            },
        ],
    },
    {
        "type": "submit",
        "defaultValue": "Save Settings"
    },
];