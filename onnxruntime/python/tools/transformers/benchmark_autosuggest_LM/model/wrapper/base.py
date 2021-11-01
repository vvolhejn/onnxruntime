import torch
import os

class BaseModelWrapper(torch.nn.Module):
    def __init__(self, model, pad_token_id=0, float_type=None, *args, **kwargs):
        super().__init__()
        self.model = model
        self.pad_token_id = pad_token_id
        self.float_type = float_type or torch.float32

    def preprocess_inputs(self, input_ids, pad_token_id=-1, float_type=torch.float32):
        '''It automatically infer `position_ids` and `attention_mask` for `GPT2LMHeadModel`

        :param pad_token_id: -1 means not padding. Raise error if `input_ids` has more than 1 sample.
        '''
        mask = (input_ids != pad_token_id)
        position_ids = mask.long().cumsum(-1) - 1
        position_ids.masked_fill_(position_ids < 0, 0)

        attention_mask = mask.to(float_type)
        
        return {'input_ids': input_ids, 'position_ids': position_ids, 'attention_mask': attention_mask}

    def get_model_inputs(self, input_ids, past, generator_state):
        model_inputs = self.preprocess_inputs(input_ids, self.pad_token_id, float_type=self.float_type)
        if past is not None:
            for k in model_inputs:
                if k in ('input_ids', 'position_ids'):
                    # input last token is needed. But mask should contains all historical input masks
                    model_inputs[k] = model_inputs[k][:, -1:]
            input_seq_index = generator_state['input_seq_index']
            for layer_past in past:
                past = tuple(layer_past.index_select(1, input_seq_index) for layer_past in past)
                model_inputs['past'] = past

        return model_inputs

    def forward(self, input_ids, model_state=None, generator_state={}):
        NotImplemented
